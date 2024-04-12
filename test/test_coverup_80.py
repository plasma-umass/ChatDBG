# file src/chatdbg/util/printer.py:30-35
# lines [31, 32, 33, 34]
# branches []

import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import MagicMock
import textwrap
import io

# Test to cover lines 31-34
def test_print_with_custom_prefix_and_output():
    chat_prefix = '> '
    out = io.StringIO()

    # Initialize the ChatDBGPrinter with mocked parameters
    printer = ChatDBGPrinter(out=out, debugger_prompt='', chat_prefix=chat_prefix, width=80)
    
    # The text to be printed
    text = "Hello, World!"
    expected_output = textwrap.indent(text, chat_prefix, lambda _: True)

    # Execute the method we want to test
    printer._print(text)
    
    # Check that the StringIO object contains the expected output
    out.seek(0)  # Move to the start of the StringIO buffer
    output = out.read()
    assert expected_output in output
    
    # Clean up
    out.close()
