import json
import sys
import time

import litellm
import llm_utils
import openai


class LiteAssistant:
    def __init__(self, instructions, model="gpt-4", timeout=30, debug=False):
        if debug:
            self._log = open(f"chatdbg.log", "w")
        else:
            self._log = None

        self._functions = {}
        self._instructions = instructions
        self._model = model

    def add_function(self, function):
        """
        Add a new function to the list of function tools.
        The function should have the necessary json spec as its pydoc string.
        """
        schema = json.loads(function.__doc__)
        assert "name" in schema, "Bad JSON in pydoc for function tool."
        self._functions[schema["name"]] = {
            "function": function,
            "schema": schema,
        }

    def _make_call(self, tool_call) -> str:
        name = tool_call.function.name
        args = json.loads(tool_call.function.arguments)
        function = self._functions[name]["function"]
        return function(**args)

    def _print_message(self, message, indent=4, wrap=120) -> None:
        def _print_to_file(file):

            tool_calls = None
            if "tool_calls" in message:
                tool_calls = message["tool_calls"]
            elif hasattr(message, "tool_calls"):
                tool_calls = message.tool_calls

            content = None
            if "content" in message:
                content = message["content"]
            elif hasattr(message, "content"):
                content = message.content

            assert bool(tool_calls) != bool(content)

            # The longest role string is 'assistant'.
            max_role_length = 9
            # We add 3 characters for the brackets and space.
            subindent = indent + max_role_length + 3

            role = message["role"].upper()
            role_indent = max_role_length - len(role)

            if tool_calls:
                print(
                    f"{' ' * indent}[{role}]{' ' * role_indent} Function calls:",
                    file=file,
                )
                for tool_call in tool_calls:
                    arguments = json.loads(tool_call.function.arguments)
                    print(
                        f"{' ' * (subindent + indent)}{tool_call.function.name}({', '.join([f'{k}={v}' for k, v in arguments.items()])})",
                        file=file,
                    )
            else:
                content = llm_utils.word_wrap_except_code_blocks(
                    content, wrap - len(role) - indent - 3
                )
                first, *rest = content.split("\n")
                print(f"{' ' * indent}[{role}]{' ' * role_indent} {first}", file=file)
                for line in rest:
                    print(f"{' ' * subindent}{line}", file=file)
            print("\n\n", file=file)

        _print_to_file(None)  # None is the default file value for print().
        if self._log:
            _print_to_file(self._log)

    def run(self, prompt: str) -> None:
        start_time = time.perf_counter()

        try:
            conversation = [
                {"role": "system", "content": self._instructions},
                {"role": "user", "content": prompt},
            ]

            for message in conversation:
                self._print_message(message)
            while True:
                tools = [
                    {"type": "function", "function": f["schema"]}
                    for f in self._functions.values()
                ]
                completion = litellm.completion(
                    model=self._model,
                    messages=conversation,
                    tools=tools,
                )

                choice = completion.choices[0]
                self._print_message(choice.message)

                if choice.finish_reason == "tool_calls":
                    responses = []
                    for tool_call in choice.message.tool_calls:
                        function_response = self._make_call(tool_call)
                        response = {
                            "tool_call_id": tool_call.id,
                            "role": "tool",
                            "name": tool_call.function.name,
                            "content": function_response,
                        }
                        responses.append(response)
                        self._print_message(response)
                    conversation.append(choice.message)
                    conversation.extend(responses)
                elif choice.finish_reason == "stop":
                    return
                else:
                    print(f"Not found: {choice.finish_reason}.")
                    sys.exit(1)

            end_time = time.perf_counter()
            elapsed_time = end_time - start_time
            print(f"Elapsed time: {elapsed_time:.2f} seconds")
            # TODO: Print tokens / cost.
        except openai.OpenAIError as e:
            print(f"*** OpenAI Error: {e}")
