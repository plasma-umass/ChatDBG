import textwrap
import argparse
import yaml
import sys
import json
import colors
import shutil
import os
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

def prompt_choice(choices, current=None):
    choices_dict = {choice[0].upper(): choice for choice in choices}
    prompt_options = "/".join([f"[{choice[0]}]{choice[1:]}" for choice in choices])
    if current == None: current = choices[0]
    prompt = colors.color(f"Pick {prompt_options} (Press enter for {current}): ", "red")

    while True:
        user_input = input(prompt).strip().upper()
        if user_input == '':
            return current
        elif user_input in choices_dict:
            return choices_dict[user_input]
        else:
            print("Invalid choice. Please try again.")

def backup_and_overwrite(file_path, new_content):
    # Create a backup file path by adding a .bak extension
    backup_path = file_path + '.bak'

    # Check if the original file exists
    if os.path.exists(file_path):
        # Copy the original file to the backup path
        shutil.copyfile(file_path, backup_path)
        print(f"Backup created at {backup_path}")

    # Write new content to the original file
    with open(file_path, 'w') as file:
        file.write(new_content)
    print(f"File {file_path} has been overwritten with new content.")


class MarkPrinter(TextPrinter):
    def __init__(self, file):
        super().__init__(file)

    def do_one(self, x):
        meta = x['meta']
        for step in x['steps']:
            self.print()
            self._do_step(step)
        self.print()
        if meta['mark'] == '?':
            print(colors.color(f"{meta['uid']}...", "red"))
            mark = prompt_choice(['Good', 'Bad', 'Useless', '?'], meta['mark'])
            meta['mark'] = mark

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
            full = json.load(log)
    else:
        with open(args.trace, 'r') as log:
            full = yaml.safe_load(log)

    xs = eval('full['+args.index+']')
    xs = xs if isinstance(xs, list) else [xs]

    for x in xs:
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

    if args.format == 'mark':
        backup_and_overwrite(args.trace, yaml.dump(full, indent=2,sort_keys=True))
