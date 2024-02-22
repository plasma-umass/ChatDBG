import textwrap
import argparse
import yaml
import sys
import json
from pathlib import Path

class TextPrinter:
    def __init__(self, file):
        self.file = file
 
    def print(self, x=''):
        print(x, file=self.file)

    def _do_message(self, x):
        text = x['output'].strip()
        if len(text) > 0:
            self.print(textwrap.indent(text, prefix='   '))

    def _do_function(self, x):
        prompt = x['input']
        assert x['output']['type'] == 'text'
        body = x['output']['output'].rstrip()
        if len(body) > 0:
            self.print(f'   (ChatDBG idbp) {prompt}')
            self.print(textwrap.indent(body, prefix='   '))

    def _do_assistant(self, x):
        for output in x['outputs']:
            if output['type'] == 'call':
                self._do_function(output)
            else:
                self._do_message(output)
            self.print()

    def _do_step(self, x):
        prompt = x['input']
        
        self.print(f'(ChatDBG idbg) {prompt}')

        output = x['output']
        if output['type'] == 'text':
                body = output['output'].rstrip()
                if len(body) > 0:
                    self.print(textwrap.indent(body, prefix = '   '))
        else:
                self._do_assistant(output)

    def do_one(self, x):
        for step in x['steps']:
            self.print()
            self._do_step(step)
        self.print()

class MarkPrinter(TextPrinter):
    def __init__(self, file):
        super().__init__(file)

    def do_one(self, x):
        for step in x['steps']:
            self.print()
            self._do_step(step)
        self.print()
        if x['meta']['mark'] == '?':
            mark = input("Mark not set: ")
            print(mark)



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="log processor")
    parser.add_argument("trace", help="the input")
    parser.add_argument("format", help="output format")
    parser.add_argument('index', nargs='?', default='-1', help='slice of trace array to take')

    args = parser.parse_args()

    trace_path = Path(args.trace)
    stem = trace_path.stem
    suffix = trace_path.suffix

    if suffix == '.json':
        with open(args.trace, 'r') as log:
            x = json.load(log)
    else:
        with open(args.trace, 'r') as log:
            x = yaml.safe_load(log)

    x = eval('x['+args.index+']')
    x = x if isinstance(x, list) else [x]

    for x in x:
        print()
        print()
        print(x['instructions'], file=sys.stdout)
        print('-' * 80)
        if args.format == 'text':
            TextPrinter(sys.stdout).do_one(x)
        elif args.format == 'json':
            print(json.dumps(x, indent=2,sort_keys=True))
        elif args.format == 'mark':
            MarkPrinter(sys.stdout).do_one(x)
        print()
        print()
