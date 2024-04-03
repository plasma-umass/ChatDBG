import argparse
import sys
import textwrap

import yaml


class LogPrinter:
    def __init__(self, file):
        self.file = file

    def print(self, x=""):
        print(x, file=self.file)

    def _do_message(self, x):
        text = x["output"].strip()
        if len(text) > 0:
            self.print(textwrap.indent(text, prefix="   "))

    def _do_function(self, x):
        prompt = x["input"].strip()
        assert x["output"]["type"] == "text"
        body = x["output"]["output"].rstrip()
        if len(body) > 0:
            self.print(f"   (ChatDBG) {prompt}")
            self.print(textwrap.indent(body, prefix="   "))

    def _do_assistant(self, x):
        for output in x["outputs"]:
            if output["type"] == "call":
                self._do_function(output)
            else:
                self._do_message(output)
            self.print()

    def _do_step(self, x):
        prompt = x["input"].strip()

        self.print(f"(ChatDBG) {prompt}")

        output = x["output"]
        if output["type"] == "text":
            body = output["output"].rstrip()
            if len(body) > 0:
                self.print(textwrap.indent(body, prefix="   "))
        else:
            self._do_assistant(output)

    def do_one(self, x):
        for step in x["steps"]:
            self.print()
            self._do_step(step)
        self.print()


def main():
    parser = argparse.ArgumentParser(description="ChatDBG log printer")
    parser.add_argument("filenames", nargs="*", help="log files to print")

    args = parser.parse_args()

    for file in args.filenames:
        with open(file, "r") as log:
            full = yaml.safe_load(log)

        for i, x in enumerate(full):
            print()
            print(f"{i} " + ("-" * 78))
            print(x["instructions"], file=sys.stdout)
            print("-" * 80)
            LogPrinter(sys.stdout).do_one(x)
            print()
            print()


if __name__ == "__main__":
    main()
