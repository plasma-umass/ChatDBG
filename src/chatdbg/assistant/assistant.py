import collections
import json
import string
import time

import openai

from .listeners import Printer
from ..util import litellm
from ..util.text import strip_ansi
from ..util.trim import sandwich_tokens, sum_messages, trim_messages


class AssistantError(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


def remove_non_printable_chars(s: str) -> str:
    printable_chars = set(string.printable)
    filtered_string = "".join(filter(lambda x: x in printable_chars, s))
    return filtered_string


def _merge_chunks(chunks):
    # Check for a final usage chunk, and merge it with the last chunk.
    if not chunks[-1].choices and chunks[-1].usage:
        chunks[-2].usage = chunks[-1].usage
        chunks.pop()

    assert all(len(chunk.choices) == 1 for chunk in chunks)

    finish_reason = chunks[-1].choices[0].finish_reason
    usage = chunks[-1].usage
    content = "".join(
        chunk.choices[0].delta.content
        for chunk in chunks
        if chunk.choices[0].delta.content  # It can be None for tool calls.
    )

    tool_chunks = [
        bit
        for chunk in chunks
        if chunk.choices[0].delta.tool_calls
        for bit in chunk.choices[0].delta.tool_calls
    ]
    tool_calls = collections.defaultdict(
        lambda: {"id": "", "name": "", "arguments": ""}
    )
    for tool_chunk in tool_chunks:
        if tool_chunk.id:
            tool_calls[tool_chunk.index]["id"] += tool_chunk.id
        if tool_chunk.function.name:
            tool_calls[tool_chunk.index]["name"] += tool_chunk.function.name
        if tool_chunk.function.arguments:
            tool_calls[tool_chunk.index]["arguments"] += tool_chunk.function.arguments

    tool_calls = [
        {
            "id": tool_call["id"],
            "function": {
                "name": tool_call["name"],
                "arguments": tool_call["arguments"],
            },
            "type": "function",
        }
        for tool_call in tool_calls.values()
    ]

    return finish_reason, content, tool_calls, usage


class Assistant:
    def __init__(
        self,
        instructions,
        model="gpt-4o",
        timeout=30,
        listeners=[Printer()],
        functions=[],
        max_call_response_tokens=2048,
    ):
        self._clients = listeners

        self._functions = {}
        for f in functions:
            self._add_function(f)

        self._model = model
        self._timeout = timeout
        self._conversation = [{"role": "system", "content": instructions}]
        self._max_call_response_tokens = max_call_response_tokens
        self._broadcast("on_begin_dialog", instructions)
        try:
            self._client = openai.OpenAI()
        except openai.OpenAIError as e:
            raise AssistantError(
                "OpenAI initialization error. Check your API settings and restart ChatDBG.\nIs OPENAI_API_KEY set?"
            )

    def close(self):
        self._broadcast("on_end_dialog")

    def _warn_about_exception(self, e, message="Unexpected Exception"):
        import traceback

        tb_lines = traceback.format_exception(type(e), e, e.__traceback__)
        tb_string = "".join(tb_lines)
        self._broadcast("on_error", f"{message}\n\n{e}\n{tb_string}")

    def query(self, prompt: str, user_text):
        """
        Send a query to the LLM.
          - prompt is the prompt to send.
          - user_text is what the user typed (which may or not be the same as prompt)

        Returns a dictionary containing:
            - "completed":          True of the query ran to completion.
            - "cost":               Cost of query, if completed. Present only if cost could be computed.
        Other fields only if completed is True
            - "time":               completion time in seconds
            - "model":              the model used
        """
        result = {"completed": False, "cost": 0}
        start = time.time()

        self._broadcast("on_begin_query", prompt, user_text)
        try:
            stats = self._streamed_query(prompt, user_text)
            elapsed = time.time() - start

            if self._model in litellm.model_data:
                model_data = litellm.model_data[self._model]
                result["cost"] = (
                    stats["prompt_tokens"] * model_data["input_cost_per_token"]
                    + stats["completion_tokens"] * model_data["output_cost_per_token"]
                )
                result["message"] = f"\n[Cost: ~${result['cost']:.2f} USD]"

            result["time"] = elapsed
            result["model"] = self._model
            result["completed"] = True
        except KeyboardInterrupt:
            # user action -- just ignore
            result["message"] = "[Chat Interrupted]"
        except openai.AuthenticationError as e:
            self._warn_about_exception(
                e, "OpenAI Error. Check your API key and restart ChatDBG."
            )
        except openai.OpenAIError as e:
            self._warn_about_exception(e, "Unexpected OpenAI Error.")
        except Exception as e:
            self._warn_about_exception(e, "Unexpected Exception.")

        self._broadcast("on_end_query", result)
        return result

    def _report(self, stats):
        if stats["completed"]:
            print()
        else:
            print("[Chat Interrupted]")

    def _broadcast(self, method_name, *args):
        for client in self._clients:
            method = getattr(client, method_name, None)
            if callable(method):
                method(*args)

    def _add_function(self, function):
        """
        Add a new function to the list of function tools.
        The function should have the necessary json spec as its docstring
        """
        schema = json.loads(function.__doc__)
        assert "name" in schema, "Bad JSON in docstring for function tool."
        self._functions[schema["name"]] = {"function": function, "schema": schema}

    def _make_call(self, tool_call) -> str:
        name = tool_call["function"]["name"]
        try:
            args = json.loads(tool_call["function"]["arguments"])
            function = self._functions[name]
            call, result = function["function"](**args)
            result = remove_non_printable_chars(strip_ansi(result).expandtabs())
            self._broadcast("on_function_call", call, result)
        except KeyboardInterrupt as e:
            raise e
        except Exception as e:
            # likely to be an exception from the code we ran, not a bug...
            result = f"Exception in function call: {e}"
            self._broadcast("on_warn", result)
        return result

    def _streamed_query(self, prompt: str, user_text):
        self._conversation.append({"role": "user", "content": prompt})
        usage = {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0}

        while True:  # break only when finish_reason == "stop"
            stream = self._stream_completion()

            try:
                self._broadcast("on_begin_stream")
                chunks = []
                for chunk in stream:
                    chunks.append(chunk)
                    # The final usage chunk will have an empty `choices` list,
                    # because of `stream_options={"include_usage": True}`.
                    if chunk.choices:
                        assert len(chunk.choices) == 1
                        if chunk.choices[0].delta.content:
                            self._broadcast(
                                "on_stream_delta", chunk.choices[0].delta.content
                            )
            finally:
                self._broadcast("on_end_stream")

            finish_reason, content, tool_calls, usage_delta = _merge_chunks(chunks)
            usage["prompt_tokens"] += usage_delta.prompt_tokens
            usage["completion_tokens"] += usage_delta.completion_tokens
            usage["total_tokens"] += usage_delta.total_tokens

            if content:
                self._conversation.append({"role": "assistant", "content": content})
                self._broadcast("on_response", content)

            if finish_reason == "tool_calls":
                self._conversation.append(
                    {"role": "assistant", "tool_calls": tool_calls}
                )
                self._add_function_results_to_conversation(tool_calls)

            if finish_reason == "stop":
                break

        return usage

    def _stream_completion(self):
        self._trim_conversation()

        # TODO: Seems like OpenAI wants to switch to a new API: client.responses.create.
        return self._client.chat.completions.create(
            model=self._model,
            messages=self._conversation,
            tools=[
                {"type": "function", "function": f["schema"]}
                for f in self._functions.values()
            ],
            timeout=self._timeout,
            stream=True,
            stream_options={"include_usage": True},
        )

    def _trim_conversation(self):
        old_len = sum_messages(self._conversation, self._model)
        self._conversation = trim_messages(self._conversation, self._model)
        new_len = sum_messages(self._conversation, self._model)

        if old_len != new_len:
            self._broadcast(
                "on_warn", f"Trimming conversation from {old_len} to {new_len} tokens."
            )

    def _add_function_results_to_conversation(self, tool_calls):
        try:
            for tool_call in tool_calls:
                function_response = self._make_call(tool_call)
                function_response = sandwich_tokens(
                    function_response, self._model, self._max_call_response_tokens, 0.5
                )
                response = {
                    "tool_call_id": tool_call["id"],
                    "role": "tool",
                    "name": tool_call["function"]["name"],
                    "content": function_response,
                }
                self._conversation.append(response)
        except Exception as e:
            # Warning: potential infinite loop if the LLM keeps sending
            # the same bad call.
            self._broadcast(
                "on_error", f"An exception occured while processing tool calls: {e}"
            )
