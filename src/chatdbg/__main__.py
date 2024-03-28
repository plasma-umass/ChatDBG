import ipdb
from chatdbg.chatdbg_pdb import ChatDBG
from chatdbg.util.config import chatdbg_config
import sys
import getopt

_usage = """\
usage: python -m ipdb [-m] [-c command] ... pyfile [arg] ...

Debug the Python program given by pyfile.

Initial commands are read from .pdbrc files in your home directory
and in the current directory, if they exist.  Commands supplied with
-c are executed after commands from .pdbrc files.

To let the script run until an exception occurs, use "-c continue".
To let the script run up to a given line X in the debugged file, use
"-c 'until X'"

Option -m is available only in Python 3.7 and later.

ChatDBG-specific options may appear anywhere before pyfile:
  --debug         dump the LLM messages to a chatdbg.log
  --log file      where to write the log of the debugging session
  --model model   the LLM model to use.
  --stream        stream responses from the LLM

"""


def main():
    ipdb.__main__._get_debugger_cls = lambda: ChatDBG

    opts, args = getopt.getopt(
        sys.argv[1:], "mhc:", ["help", "debug", "log=", "model=", "stream", "command="]
    )
    pdb_args = [sys.argv[0]]
    for opt, optarg in opts:
        if opt in ["-h", "--help"]:
            print(_usage)
            sys.exit()
        elif opt in ["--debug"]:
            chatdbg_config.debug = True
        elif opt in ["--stream"]:
            chatdbg_config.stream = True
        elif opt in ["--model"]:
            chatdbg_config.model = optarg
        elif opt in ["--log"]:
            chatdbg_config.model = optarg
        elif opt in ["-c", "--command"]:
            pdb_args += [opt, optarg]
        elif opt in ["-m"]:
            pdb_args = [opt]

    if not args:
        print(_usage)
        sys.exit(2)

    sys.argv = pdb_args + args

    ipdb.__main__.main()
