import textwrap
from ..assistant.assistant import AssistantPrinter
from ..chatdbg_pdb import ChatDBG
import sys


class Printer(AssistantPrinter):

    def __init__(self, message, error, log):
        self._message = message
        self._error = error
        self._log = log

    def stream(self, text):
        print(text, flush=True, end=None)

    def message(self, text):
        print(text, flush=True)

    def log(self, json_obj):
        pass

    def fail(self, message):
        print()
        print(textwrap.wrap(message, width=70, initial_indent="*** "))
        sys.exit(1)

    def warn(self, message):
        print()
        print(textwrap.wrap(message, width=70, initial_indent="*** "))


class StreamingPrinter(AssistantPrinter):

    def __init__(self, message, error):
        self.message = message
        self.error = error

    def stream(self, text):
        print(text, flush=True, end=None)

    def message(self, text):
        print(text, flush=True)

    def log(self, json_obj):
        pass

    def fail(self, message):
        print()
        print(textwrap.wrap(message, width=70, initial_indent="*** "))
        sys.exit(1)

    def warn(self, message):
        print()
        print(textwrap.wrap(message, width=70, initial_indent="*** "))
