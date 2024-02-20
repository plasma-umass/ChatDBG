import atexit
import inspect
import json
import textwrap
import time
import sys

import llm_utils
from openai import *
from pydantic import BaseModel


class Assistant:
    """
    An Assistant is a wrapper around OpenAI's assistant API.  Example usage:

        assistant = Assistant("Assistant Name", instructions,
                              model='gpt-4-1106-preview', debug=True)
        assistant.add_function(my_func)
        response = assistant.run(user_prompt)

    Name can be any name you want.

    If debug is True, it will create a log of all messages and JSON responses in
    json.txt.
    """

    def __init__(self, name, instructions, model="gpt-3.5-turbo-1106", debug=True):
        if debug:
            self.json = open(f"json.txt", "w")
        else:
            self.json = None

        try:
            self.client = OpenAI(timeout=30)
        except OpenAIError:
            print(
                textwrap.dedent(
                    """\
            You need an OpenAI key to use this tool.
            You can get a key here: https://platform.openai.com/api-keys
            Set the environment variable OPENAI_API_KEY to your key value.
            """
                )
            )
            sys.exit(0)

        self.assistants = self.client.beta.assistants
        self.threads = self.client.beta.threads
        self.functions = dict()

        self.assistant = self.assistants.create(
            name=name, instructions=instructions, model=model
        )

        self._log(self.assistant)

        atexit.register(self._delete_assistant)

        self.thread = self.threads.create()
        self._log(self.thread)

    def _delete_assistant(self):
        if self.assistant != None:
            try:
                id = self.assistant.id
                response = self.assistants.delete(id)
                self._log(response)
                assert response.deleted
            except Exception as e:
                print(
                    f"Assistant {id} was not deleted ({e}).\nYou can do so at https://platform.openai.com/assistants."
                )

    def add_function(self, function):
        """
        Add a new function to the list of function tools for the assistant.
        The function should have the necessary json spec as is pydoc string.
        """
        function_json = json.loads(function.__doc__)
        assert "name" in function_json, "Bad JSON in pydoc for function tool."
        try:
            name = function_json["name"]
            self.functions[name] = function

            tools = [
                {"type": "function", "function": json.loads(function.__doc__)}
                for function in self.functions.values()
            ]

            assistant = self.assistants.update(self.assistant.id, tools=tools)
            self._log(assistant)
        except OpenAIError as e:
            print(f"*** OpenAI Error: {e}")

    def _make_call(self, tool_call):
        name = tool_call.function.name
        args = tool_call.function.arguments

        # There is a sketchy case that happens occasionally because
        # the API produces a bad call...
        try:
            args = json.loads(args)
            function = self.functions[name]
            result = function(**args)
        except Exception as e:
            result = f"Ill-formed function call ({e})\n"

        return result

    def _print_messages(self, messages, client_print):
        client_print()
        for i, m in enumerate(messages):
            message_text = m.content[0].text.value
            if i == 0:
                message_text = "(Message) " + message_text
            client_print(message_text)

    def _wait_on_run(self, run, thread, client_print):
        try:
            while run.status == "queued" or run.status == "in_progress":
                run = self.threads.runs.retrieve(
                    thread_id=thread.id,
                    run_id=run.id,
                )
                time.sleep(0.5)
            return run
        finally:
            if run.status == "in_progress":
                client_print("Cancelling message that's in progress.")
                self.threads.runs.cancel(thread_id=thread.id, run_id=run.id)

    def run(self, prompt, client_print=print):
        """
        Give the prompt to the assistant and get the response, which may included
        intermediate function calls.
        All output is printed to the given file.
        """
        start_time = time.perf_counter()

        try:
            if self.assistant == None:
                return 0, 0, 0

            assert len(prompt) <= 32768

            message = self.threads.messages.create(
                thread_id=self.thread.id, role="user", content=prompt
            )
            self._log(message)

            last_printed_message_id = message.id

            run = self.threads.runs.create(
                thread_id=self.thread.id, assistant_id=self.assistant.id
            )
            self._log(run)

            run = self._wait_on_run(run, self.thread, client_print)
            self._log(run)

            while run.status == "requires_action":
                messages = self.threads.messages.list(
                    thread_id=self.thread.id, after=last_printed_message_id, order="asc"
                )

                mlist = list(messages)
                if len(mlist) > 0:
                    self._print_messages(mlist, client_print)
                    last_printed_message_id = mlist[-1].id
                    client_print()

                outputs = []
                for tool_call in run.required_action.submit_tool_outputs.tool_calls:
                    output = self._make_call(tool_call)
                    self._log(output)
                    outputs += [{"tool_call_id": tool_call.id, "output": output}]

                try:
                    run = self.threads.runs.submit_tool_outputs(
                        thread_id=self.thread.id, run_id=run.id, tool_outputs=outputs
                    )
                    self._log(run)
                except Exception as e:
                    self._log(run, f"FAILED to submit tool call results: {e}")

                run = self._wait_on_run(run, self.thread, client_print)
                self._log(run)

            if run.status == "failed":
                message = f"\n**Internal Failure ({run.last_error.code}):** {run.last_error.message}"
                client_print(message)
                return 0, 0, 0

            messages = self.threads.messages.list(
                thread_id=self.thread.id, after=last_printed_message_id, order="asc"
            )
            self._print_messages(messages, client_print)

            end_time = time.perf_counter()
            elapsed_time = end_time - start_time

            cost = llm_utils.calculate_cost(
                run.usage.prompt_tokens,
                run.usage.completion_tokens,
                self.assistant.model,
            )
            client_print()
            client_print(f"[Cost: ~${cost:.2f} USD]")
            return run.usage.total_tokens, cost, elapsed_time
        except OpenAIError as e:
            client_print(f"*** OpenAI Error: {e}")
            return 0, 0, 0

    def _log(self, obj, title=""):
        if self.json != None:
            stack = inspect.stack()
            caller_frame_record = stack[1]
            lineno, function = caller_frame_record[2:4]
            loc = f"{function}:{lineno}"

            print("-" * 70, file=self.json)
            print(f"{loc}  {title}", file=self.json)
            if isinstance(obj, BaseModel):
                json_obj = json.loads(obj.model_dump_json())
            else:
                json_obj = obj
            print(f"\n{json.dumps(json_obj, indent=2)}\n", file=self.json)
            self.json.flush()
        return obj
