
import textwrap
from ..assistant.listeners import BaseAssistantListener
from .stream import StreamingTextWrapper
from .wrap import word_wrap_except_code_blocks


class ChatDBGPrinter(BaseAssistantListener):
    def __init__(self, out, debugger_prompt, chat_prefix, width, stream=False):
        self.out = out
        self.debugger_prompt = debugger_prompt
        self.chat_prefix = chat_prefix
        self.width = width
        self._assistant = None
        self._stream = stream

    # Call backs

    def on_begin_query(self, prompt, user_text):
        pass

    def on_end_query(self, stats):
        pass

    def _print(self, text, **kwargs):
        print(
            textwrap.indent(text, self.chat_prefix, lambda _: True),
            file=self.out,
            **kwargs,
        )

    def on_warn(self, text):
        print(textwrap.indent(text, "*** "), file=self.out)

    def on_fail(self, text):
        print(textwrap.indent(text, "*** "), file=self.out)

    def on_begin_stream(self):
        self._stream_wrapper = StreamingTextWrapper(self.chat_prefix, width=80)
        self._at_start = True

    def on_stream_delta(self, text):
        if self._at_start:
            self._at_start = False
            print(
                self._stream_wrapper.append("\n(Message) ", False),
                end="",
                flush=True,
                file=self.out,
            )
        print(
            self._stream_wrapper.append(text, False), end="", flush=True, file=self.out
        )

    def on_end_stream(self):
        print(self._stream_wrapper.flush(), end="", flush=True, file=self.out)

    def on_response(self, text):
        if not self._stream and text != None:
            text = word_wrap_except_code_blocks(
                text, self.width - len(self.chat_prefix)
            )
            self._print(text)

    def on_function_call(self, call, result):
        if result and len(result) > 0:
            entry = f"{self.debugger_prompt}{call}\n{result}"
        else:
            entry = f"{self.debugger_prompt}{call}"
        self._print(entry)
