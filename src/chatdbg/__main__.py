import os
import pathlib
import sys

the_path = pathlib.Path(__file__).parent.resolve()

sys.path.insert(0, os.path.abspath(the_path))

from . import chatdbg

chatdbg.main()
