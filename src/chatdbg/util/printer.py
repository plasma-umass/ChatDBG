import textwrap
from ..assistant.listeners import BaseAssistantListener
from .stream import StreamingTextWrapper
from .wrap import word_wrap_except_code_blocks
import os


class ChatDBGPrinter(BaseAssistantListener):
    def __init__(self, out, debugger_prompt, chat_prefix, width):
        self._out = out
        self._debugger_prompt = debugger_prompt
        self._chat_prefix = chat_prefix
        try:
            self._width = min(width, os.get_terminal_size().columns - len(chat_prefix))
        except:
            # get_terminal_size() may fail in notebooks
            self._width = width

        # used to keep track of streaming
        self._at_start = True

    # Call backs

    def on_begin_query(self, prompt, user_text):
        pass

    def on_end_query(self, stats):
        pass

    def _print(self, text, **kwargs):
        print(
            textwrap.indent(text, self._chat_prefix, lambda _: True),
            file=self._out,
            **kwargs,
        )

    def on_warn(self, text):
        print(textwrap.indent(text, "*** "), file=self._out)

    def on_error(self, text):
        print(textwrap.indent(text, "*** "), file=self._out)

    def on_begin_stream(self):
        self._stream_wrapper = StreamingTextWrapper(
            self._chat_prefix, width=self._width
        )
        self._at_start = True

    def on_stream_delta(self, text):
        if self._at_start:
            self._at_start = False
            print(
                self._stream_wrapper.append("\n(Message) ", False),
                end="",
                flush=True,
                file=self._out,
            )
        print(
            self._stream_wrapper.append(text, False), end="", flush=True, file=self._out
        )

    def on_end_stream(self):
        print(self._stream_wrapper.flush(), end="", flush=True, file=self._out)
        self._at_start = True

    def on_response(self, text):
        if self._at_start and text != None:
            text = "(Message) " + text
            text = word_wrap_except_code_blocks(
                text, self._width - len(self._chat_prefix)
            )
            self._print(text)

    def on_function_call(self, call, result):
        if result and len(result) > 0:
            entry = f"{self._debugger_prompt}{call}\n{result}"
        else:
            entry = f"{self._debugger_prompt}{call}"
        self._print(entry)
