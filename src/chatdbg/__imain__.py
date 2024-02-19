import ipdb

from . import chatdbg_ipdb

ipdb.__main__._get_debugger_cls = lambda : chatdbg_ipdb.ChatDBG

def main():
    ipdb.__main__.main()
