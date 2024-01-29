#! /usr/bin/env python3

import importlib
import pdb
import sys

from . import chatdbg_why


class ChatDBG(pdb.Pdb):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.prompt = "(ChatDBG Pdb) "

    def do_why(self, arg):
        chatdbg_why.why(self, arg)


_usage = f"""\
usage: chatdbg [-c command] ... [-m module | pyfile] [arg] ...

A Python debugger that uses AI to tell you `why`.
(version {importlib.metadata.metadata('ChatDBG')['Version']})

https://github.com/plasma-umass/ChatDBG

Debug the Python program given by pyfile. Alternatively,
an executable module or package to debug can be specified using
the -m switch.

Initial commands are read from .pdbrc files in your home directory
and in the current directory, if they exist.  Commands supplied with
-c are executed after commands from .pdbrc files.

To let the script run until an exception occurs, use "-c continue".
You can then type `why` to get an explanation of the root cause of
the exception, along with a suggested fix. NOTE: you must have an
OpenAI key saved as the environment variable OPENAI_API_KEY.
You can get a key here: https://openai.com/api/

To let the script run up to a given line X in the debugged file, use
"-c 'until X'"."""


def main():
    import getopt

    opts, args = getopt.getopt(sys.argv[1:], "mhc:", ["help", "command="])

    if any(opt in ["-h", "--help"] for opt, optarg in opts):
        print(_usage)
        sys.exit()

    if not args:
        print(_usage)
        sys.exit(2)

    pdb.Pdb = ChatDBG
    pdb.main()