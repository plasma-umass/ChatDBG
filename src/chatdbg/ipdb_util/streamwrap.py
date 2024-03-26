import textwrap
import re
import sys
from .text import word_wrap_except_code_blocks


class StreamingTextWrapper:

    def __init__(self, indent='  ', width=70):
        self.buffer = ''   # the raw text so far
        self.wrapped = ''  # the successfully wrapped text do far
        self.pending = ''  # the part after the last space in buffer -- has not been wrapped yet
        self.indent = indent
        self.width = width

    def add(self, text, flush=False):
        if flush:
            self.buffer += self.pending + text
            self.pending = ''
        else:
            text_bits = re.split(r'(\s+)', self.pending + text)
            self.pending = text_bits[-1]
            self.buffer += (''.join(text_bits[0:-1]))

        wrapped = word_wrap_except_code_blocks(self.buffer, self.width)
        wrapped = textwrap.indent(wrapped, self.indent, lambda _: True)
        wrapped_delta = wrapped[len(self.wrapped):]
        self.wrapped = wrapped
        return wrapped_delta

    def flush(self):
        result = self.add('', flush=True)
        self.buffer = ''
        self.wrapped = '' 
        return result



if __name__ == '__main__':
    s = StreamingTextWrapper(3,20)
    for x in sys.argv[1:]:
        y = s.add(' ' + x)
        print(y, end='', flush=True)
    print(s.flush())
