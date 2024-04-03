import ipdb
from chatdbg.chatdbg_pdb import ChatDBG
from chatdbg.util.config import chatdbg_config
import sys

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
"""


def main() -> None:
    ipdb.__main__._get_debugger_cls = lambda: ChatDBG

    args = chatdbg_config.parse_user_flags(sys.argv[1:])

    if "-h" in args or "--help" in args:
        print(_usage)
        print(chatdbg_config.user_flags_help())
        sys.exit()

    sys.argv = [sys.argv[0]] + args

    ipdb.__main__.main()


if __name__ == "__main__":
    main()
