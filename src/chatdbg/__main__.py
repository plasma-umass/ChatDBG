from .chatdbg_pdb import *
import ipdb


def main():
    ipdb.__main__._get_debugger_cls = lambda: ChatDBG
    ipdb.__main__.main()
