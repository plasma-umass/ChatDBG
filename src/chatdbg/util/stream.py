import textwrap
import re
import sys
from .wrap import word_wrap_except_code_blocks


class StreamingTextWrapper:

    def __init__(self, indent="  ", width=80):
        self._buffer = ""  # the raw text so far
        self._wrapped = ""  # the successfully wrapped text do far
        self._pending = (
            ""  # the part after the last space in buffer -- has not been wrapped yet
        )
        self._indent = indent
        self._width = width - len(indent)

    def append(self, text, flush=False):
        if flush:
            self._buffer += self._pending + text
            self._pending = ""
        else:
            text_bits = re.split(r"(\s+)", self._pending + text)
            self._pending = text_bits[-1]
            self._buffer += "".join(text_bits[0:-1])

        wrapped = word_wrap_except_code_blocks(self._buffer, width=self._width)
        wrapped = textwrap.indent(wrapped, self._indent, lambda _: True)
        wrapped_delta = wrapped[len(self._wrapped) :]
        self._wrapped = wrapped
        return wrapped_delta

    def flush(self):
        if len(self._buffer) > 0:
            result = self.append("\n", flush=True)
        else:
            result = self.append("", flush=True)
        self._buffer = ""
        self._wrapped = ""
        return result


if __name__ == "__main__":
    s = StreamingTextWrapper(3, 20)
    for x in sys.argv[1:]:
        y = s.append(" " + x)
        print(y, end="", flush=True)
    print(s.flush())
