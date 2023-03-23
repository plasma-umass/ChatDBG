#! /usr/bin/env python3

from . import chatdbg_pdb
chatdbg = chatdbg_pdb

# When invoked as main program, invoke the debugger on a script
if __name__ == "__main__":
    chatdbg.main()
