# file src/chatdbg/util/printer.py:40-41
# lines [40, 41]
# branches []

import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import MagicMock
import textwrap
import io

@pytest.fixture
def mock_stdout():
    return io.StringIO()

def test_on_error_with_mock_stdout(mock_stdout):
    printer = ChatDBGPrinter(out=mock_stdout, debugger_prompt="", chat_prefix="", width=80)
    error_message = "Test error message"
    expected_output = textwrap.indent(error_message, "*** ") + "\n"
    
    printer.on_error(error_message)
    
    assert mock_stdout.getvalue() == expected_output
