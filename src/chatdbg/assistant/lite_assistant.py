import json
import sys
import time
from typing import Callable

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
        self._timeout = timeout

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

    def _print_message(
        self, message, indent, append_message: Callable[[str], None], wrap=120
    ) -> None:
        def _format_message(indent) -> str:
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

            assert content != None or tool_calls != None

            # The longest role string is 'assistant'.
            max_role_length = 9
            # We add 3 characters for the brackets and space.
            subindent = indent + max_role_length + 3

            role = message["role"].upper()
            role_indent = max_role_length - len(role)

            output = ""

            if content != None:
                content = llm_utils.word_wrap_except_code_blocks(
                    content, wrap - len(role) - indent - 3
                )
                first, *rest = content.split("\n")
                output += f"{' ' * indent}[{role}]{' ' * role_indent} {first}\n"
                for line in rest:
                    output += f"{' ' * subindent}{line}\n"

            if tool_calls != None:
                if content != None:
                    output += f"{' ' * subindent} Function calls:\n"
                else:
                    output += (
                        f"{' ' * indent}[{role}]{' ' * role_indent} Function calls:\n"
                    )
                for tool_call in tool_calls:
                    arguments = json.loads(tool_call.function.arguments)
                    output += f"{' ' * (subindent + 4)}{tool_call.function.name}({', '.join([f'{k}={v}' for k, v in arguments.items()])})\n"
            return output

        append_message(_format_message(indent))
        if self._log:
            print(_format_message(0), file=self._log)

    def run(
        self,
        prompt: str,
        append_message: Callable[[str], None] = print,
        append_warning: Callable[[str], None] = print,
        set_error: Callable[[str], None] = print,
    ) -> None:
        start = time.time()
        cost = 0

        try:
            conversation = [
                {"role": "system", "content": self._instructions},
                {"role": "user", "content": prompt},
            ]

            for message in conversation:
                self._print_message(message, 4, append_message)
            while True:
                completion = litellm.completion(
                    model=self._model,
                    messages=conversation,
                    tools=[
                        {"type": "function", "function": f["schema"]}
                        for f in self._functions.values()
                    ],
                    timeout=self._timeout,
                )

                cost += litellm.completion_cost(completion)

                choice = completion.choices[0]
                self._print_message(choice.message, 4, append_message)

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
                        self._print_message(response, 4, append_message)
                    conversation.append(choice.message)
                    conversation.extend(responses)
                elif choice.finish_reason == "stop":
                    break
                else:
                    set_error(f"not found: {choice.finish_reason}.")
                    return

            elapsed = time.time() - start
            append_message(
                f"Elapsed time: {elapsed:.2f} seconds\nTotal cost: {cost:.2f}$"
            )
        except openai.OpenAIError as e:
            set_error(e)
