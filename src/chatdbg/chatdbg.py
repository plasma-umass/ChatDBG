#! /usr/bin/env python3

from . import pdb
from .pdb import Pdb, Restart, _ModuleTarget, _ScriptTarget

import asyncio
import sys
import traceback

from . import chatdbg_why


class ChatDBG(Pdb):
    def do_why(self, arg):
        asyncio.run(chatdbg_why.why(self, arg))


import importlib.metadata

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

    if not args:
        print(_usage)
        sys.exit(2)

    if any(opt in ["-h", "--help"] for opt, optarg in opts):
        print(_usage)
        sys.exit()

    commands = [optarg for opt, optarg in opts if opt in ["-c", "--command"]]

    module_indicated = any(opt in ["-m"] for opt, optarg in opts)
    cls = _ModuleTarget if module_indicated else _ScriptTarget
    target = cls(args[0])

    target.check()

    sys.argv[:] = args  # Hide "pdb.py" and pdb options from argument list

    # Note on saving/restoring sys.argv: it's a good idea when sys.argv was
    # modified by the script being debugged. It's a bad idea when it was
    # changed by the user from the command line. There is a "restart" command
    # which allows explicit specification of command line arguments.
    pdb = ChatDBG()
    pdb.rcLines.extend(commands)
    while True:
        try:
            pdb._run(target)
            if pdb._user_requested_quit:
                break
            print("The program finished and will be restarted")
        except Restart:
            print("Restarting", target, "with arguments:")
            print("\t" + " ".join(sys.argv[1:]))
        except SystemExit:
            # In most cases SystemExit does not warrant a post-mortem session.
            print("The program exited via sys.exit(). Exit status:", end=" ")
            print(sys.exc_info()[1])
        except SyntaxError:
            traceback.print_exc()
            sys.exit(1)
        except:
            traceback.print_exc()
            print("Uncaught exception. Entering post mortem debugging")
            print("Running 'cont' or 'step' will restart the program")
            t = sys.exc_info()[2]
            pdb.interaction(None, t)
            print("Post mortem debugger finished. The " + target + " will be restarted")


# When invoked as main program, invoke the debugger on a script
if __name__ == "__main__":
    import chatdbg

    chatdbg.main()
