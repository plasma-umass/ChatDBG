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
        def _print_to_file(file, indent):

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

            assert content or tool_calls

            # The longest role string is 'assistant'.
            max_role_length = 9
            # We add 3 characters for the brackets and space.
            subindent = indent + max_role_length + 3

            role = message["role"].upper()
            role_indent = max_role_length - len(role)

            if content:
                content = llm_utils.word_wrap_except_code_blocks(
                    content, wrap - len(role) - indent - 3
                )
                first, *rest = content.split("\n")
                print(f"{' ' * indent}[{role}]{' ' * role_indent} {first}", file=file)
                for line in rest:
                    print(f"{' ' * subindent}{line}", file=file)
                print()

            if tool_calls:
                if content:
                    print(f"{' ' * subindent} Function calls:", file=file)
                else:
                    print(
                        f"{' ' * indent}[{role}]{' ' * role_indent} Function calls:",
                        file=file,
                    )
                for tool_call in tool_calls:
                    arguments = json.loads(tool_call.function.arguments)
                    print(
                        f"{' ' * (subindent + 4)}{tool_call.function.name}({', '.join([f'{k}={v}' for k, v in arguments.items()])})",
                        file=file,
                    )
                print()
            print("\n", file=file)

        # None is the default file value for print().
        _print_to_file(None, indent)
        if self._log:
            _print_to_file(self._log, 0)

    def run(self, prompt: str) -> None:
        start = time.time()
        cost = 0

        try:
            conversation = [
                {"role": "system", "content": self._instructions},
                {"role": "user", "content": prompt},
            ]

            for message in conversation:
                self._print_message(message)
            while True:
                completion = litellm.completion(
                    model=self._model,
                    messages=conversation,
                    tools=[
                        {"type": "function", "function": f["schema"]}
                        for f in self._functions.values()
                    ],
                )

                cost += litellm.completion_cost(completion)

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
                    break
                else:
                    print(f"Not found: {choice.finish_reason}.")
                    sys.exit(1)

            elapsed = time.time() - start
            print(f"Elapsed time: {elapsed:.2f} seconds")
            print(f"Total cost: {cost:.2f}$")
        except openai.OpenAIError as e:
            print(f"*** OpenAI Error: {e}")
