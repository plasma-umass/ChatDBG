import textwrap
import argparse
import yaml
import sys
import json
from pathlib import Path

def do_message(x, file):
    text = x['output'].strip()
    if len(text) > 0:
        print(textwrap.indent(text, prefix='   '), file=file)

def do_function(x, file):
    prompt = x['input']
    assert x['output']['type'] == 'text'
    body = x['output']['output'].rstrip()
    if len(body) > 0:
        print(f'   (ChatDBG idbp) {prompt}', file=file)
        print(textwrap.indent(body, prefix='   '), file=file)


def do_assistant(x, file):
    for output in x['outputs']:
        if output['type'] == 'call':
            do_function(output, file)
        else:
            do_message(output, file)
        print(file=file)

def do_step(file, x):
    prompt = x['input']
    
    print(f'(ChatDBG idbg) {prompt}', file=file)

    output = x['output']
    if output['type'] == 'text':
            body = output['output'].rstrip()
            if len(body) > 0:
                print(textwrap.indent(body, prefix = '   '), file=file)
    else:
            do_assistant(output, file)

def do_trace(file, x):
    for step in x['steps']:
        print(file=sys.stdout)
        do_step(file, step)
    print(file=sys.stdout)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="log -> text")
    parser.add_argument("trace", help="the input")
    parser.add_argument('index', nargs='?', default='-1', help='slice of trace array to take')

    args = parser.parse_args()

    trace_path = Path(args.trace)
    stem = trace_path.stem
    suffix = trace_path.suffix

    if suffix == '.json':
        with open(args.trace, 'r') as input:
            x = json.load(input)
    else:
        with open(args.trace, 'r') as input:
            x = yaml.safe_load(input)

    x = eval('x['+args.index+']')
    x = x if isinstance(x, list) else [x]

    for x in x:
        print()
        print()
        print(x['instructions'], file=sys.stdout)
        print('-' * 80)
        do_trace(sys.stdout, x)
        print()
        print()
