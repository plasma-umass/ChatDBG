import textwrap
import re
import sys
from llm_utils import word_wrap_except_code_blocks


class StreamTextWrapper:

    def __init__(self, indent='  ', width=70):
        self.buffer = ''
        self.wrapped = ''
        self.pending = ''
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

        # print('---', self.buffer, '---', self.pending)
        wrapped = word_wrap_except_code_blocks(self.buffer, self.width)
        wrapped = textwrap.indent(wrapped, self.indent, lambda _: True)
        printable_part = wrapped[len(self.wrapped):]
        self.wrapped = wrapped
        return printable_part

    def flush(self):
        return self.add('', flush=True)



if __name__ == '__main__':
    s = StreamTextWrapper(3,20)
    for x in sys.argv[1:]:
        y = s.add(' ' + x)
        print(y, end='', flush=True)
    print(s.flush())
