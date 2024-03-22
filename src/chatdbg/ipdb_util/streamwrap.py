import textwrap
import re
import sys
import llm_utils


class StreamTextWrapper:

    def __init__(self, indent=3, width=70):
        self.buffer = ''
        self.wrapped = ''
        self.pending = ''
        self.indent = indent
        self.width = width

    def add(self, text, flush=False):
        unwrapped = self.buffer
        if flush:
            unwrapped += self.pending + text
            self.pending = ''
        else:
            text_bits = re.split(r'(\s+)', self.pending + text)
            self.pending = text_bits[-1]
            unwrapped += (''.join(text_bits[0:-1]))

        # print('---', unwrapped, '---', self.pending)
        wrapped = word_wrap_except_code_blocks(unwrapped, self.width)
        wrapped = textwrap.indent(wrapped, ' ' * self.indent, lambda _: True)
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
