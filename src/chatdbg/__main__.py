import sys

import ipdb

from chatdbg.chatdbg_pdb import ChatDBG
from chatdbg.util.config import chatdbg_config
from chatdbg.util.help import print_help


def main() -> None:
    ipdb.__main__._get_debugger_cls = lambda: ChatDBG

    args = chatdbg_config.parse_user_flags(sys.argv[1:])

    if "-h" in args or "--help" in args:
        print_help()

    sys.argv = [sys.argv[0]] + args

    ipdb.__main__.main()


if __name__ == "__main__":
    main()
