# file src/chatdbg/util/jupyter.py:11-12
# lines [11, 12]
# branches []

import pytest
from chatdbg.util.jupyter import ChatDBGJupyterPrinter

# The following test should exercise the __init__ method of 'ChatDBGJupyterPrinter'
# and ensure that the initialization of the object is correct.

def test_chat_dbg_jupyter_printer_initialization():
    debugger_prompt = "debugger>"
    chat_prefix = "chat>"
    width = 80

    # Instantiate the ChatDBGJupyterPrinter
    printer = ChatDBGJupyterPrinter(debugger_prompt, chat_prefix, width)

    # Assertions to ensure the object is an instance of ChatDBGJupyterPrinter
    assert isinstance(printer, ChatDBGJupyterPrinter)

# No need to define a separate test suite, pytest will automatically discover and run the test.
