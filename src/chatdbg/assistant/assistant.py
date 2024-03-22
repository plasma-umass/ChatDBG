import atexit
import textwrap
import json
import time
import sys

import llm_utils
from openai import *
from pydantic import BaseModel

class AssistantPrinter:
    def text_delta(self, text=''):
        print(text, flush=True, end='')

    def text_message(self, text=''):
        print(text, flush=True)

    def log(self, json_obj):
        pass

    def fail(self, message='Failed'):
        print()
        print(textwrap.wrap(message, width=70, initial_indent='*** '))
        sys.exit(1)
        
    def warn(self, message='Warning'):
        print()
        print(textwrap.wrap(message, width=70, initial_indent='*** '))
        

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

    def __init__(
        self, name, instructions, model="gpt-3.5-turbo-1106", timeout=30,
        printer = AssistantPrinter()):
        self.printer = printer
        try:
            self.client = OpenAI(timeout=timeout)
        except OpenAIError:
            self.printer.fail("""\
                You need an OpenAI key to use this tool.
                You can get a key here: https://platform.openai.com/api-keys.
                Set the environment variable OPENAI_API_KEY to your key value.""")

        self.assistants = self.client.beta.assistants
        self.threads = self.client.beta.threads
        self.functions = dict()

        self.assistant = self.assistants.create(
            name=name, instructions=instructions, model=model
        )
        self.thread = self.threads.create()

        atexit.register(self._delete_assistant)

    def _delete_assistant(self):
        if self.assistant != None:
            try:
                id = self.assistant.id
                response = self.assistants.delete(id)
                assert response.deleted
            except OSError:
                raise
            except Exception as e:
                self.printer.warn(f"Assistant {id} was not deleted ({e}).  You can do so at https://platform.openai.com/assistants.")

    def add_function(self, function):
        """
        Add a new function to the list of function tools for the assistant.
        The function should have the necessary json spec as its pydoc string.
        """
        function_json = json.loads(function.__doc__)
        try:
            name = function_json["name"]
            self.functions[name] = function

            tools = [
                {"type": "function", "function": json.loads(function.__doc__)}
                for function in self.functions.values()
            ]

            self.assistants.update(self.assistant.id, tools=tools)
        except OpenAIError as e:
            self.printer.fail(f"*** OpenAI Error: {e}")

    def _make_call(self, tool_call):
        name = tool_call.function.name
        args = tool_call.function.arguments

        # There is a sketchy case that happens occasionally because
        # the API produces a bad call...
        try:
            args = json.loads(args)
            function = self.functions[name]
            result = function(**args)
        except OSError as e:
            result = f"Error: {e}"
        except Exception as e:
            result = f"Ill-formed function call: {e}"

        return result

    def drain_stream(self, stream):
        run = None
        for event in stream:
            self.printer.log(event)
            if event.event == 'thread.run.completed':
                run = event.data
            if event.event == 'thread.message.delta':
                self.printer.text_delta(event.data.delta.content[0].text.value)
            if event.event == 'thread.message.completed':
                self.printer.text_message(event.data.content[0].text.value)
            elif event.event == 'thread.run.requires_action':
                r = event.data
                if r.status == "requires_action":
                    outputs = []
                    for tool_call in r.required_action.submit_tool_outputs.tool_calls:
                        output = self._make_call(tool_call)
                        outputs += [{"tool_call_id": tool_call.id, "output": output}]

                    try:
                        new_stream = self.threads.runs.submit_tool_outputs(
                            thread_id=self.thread.id, run_id=r.id, tool_outputs=outputs, stream=True
                        )
                        return self.drain_stream(new_stream)
                    except OSError as e:
                        raise
                    except Exception as e:
                        # silent failure because the tool call submit biffed.  Not muchw e can do
                        pass
            elif event.event == 'thread.run.failed':
                run = event.data
                self.printer.fail(f"*** Internal Failure ({run.last_error.code}): {run.last_error.message}")
            elif event.event == 'error':
                self.printer.fail(f"*** Internal Failure:** {event.data}")
        return run
        
    def run(self, prompt):
        """
        Give the prompt to the assistant and get the response, which may included
        intermediate function calls.
        All output is printed to the given file.
        """

        if self.assistant == None:
            return {
                "tokens": 0,
                "prompt_tokens": 0,
                "completion_tokens": 0,
                "model": self.assistant.model,
                "cost": 0,
            }

        start_time = time.perf_counter()
        
        assert len(prompt) <= 32768
        self.threads.messages.create(
            thread_id=self.thread.id, role="user", content=prompt
        )
        with self.threads.runs.create(
            thread_id=self.thread.id,
            assistant_id=self.assistant.id,
            stream=True
        ) as stream:
            run = self.drain_stream(stream)

        end_time = time.perf_counter()
        elapsed_time = end_time - start_time

        cost = llm_utils.calculate_cost(
            run.usage.prompt_tokens,
            run.usage.completion_tokens,
            self.assistant.model,
        )
        return {
            "tokens": run.usage.total_tokens,
            "prompt_tokens": run.usage.prompt_tokens,
            "completion_tokens": run.usage.completion_tokens,
            "model": self.assistant.model,
            "cost": cost,
            "time": elapsed_time,
            "thread.id": self.thread.id,
            "thread": self.thread,
            "run.id": run.id,
            "run": run,
            "assistant.id": self.assistant.id,
        }


if __name__ == '__main__':
    def weather(location):
        """
        {
            "name": "get_weather",
            "description": "Determine weather in my location",
            "parameters": {
                "type": "object",
                "properties": {
                "location": {
                    "type": "string",
                    "description": "The city and state e.g. San Francisco, CA"
                },
                "unit": {
                    "type": "string",
                    "enum": [
                    "c",
                    "f"
                    ]
                }
                },
                "required": [
                "location"
                ]
            }
        }
        """
        return "Sunny and 72 degrees."

    a = Assistant("Test", "You generate text.")
    a.add_function(weather)
    x = a.run("What's the weather in Boston?")
    print(x)