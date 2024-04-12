# file src/chatdbg/util/printer.py:37-38
# lines [38]
# branches []

import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import MagicMock
import textwrap
import io

# Assuming BaseAssistantListener is part of the module, if not, it should be imported or mocked.

class TestChatDBGPrinter:

    @pytest.fixture
    def printer(self, monkeypatch):
        # Setup a printer with a mock output stream to capture prints
        mock_out = io.StringIO()
        monkeypatch.setattr('sys.stdout', mock_out)
        # Assuming the constructor of ChatDBGPrinter requires 'out', 'debugger_prompt', 'chat_prefix', and 'width'
        # and that they can be mocked or set to default values for the purpose of this test.
        printer = ChatDBGPrinter(out=mock_out, debugger_prompt='dbg>', chat_prefix='chat>', width=80)
        return printer

    def test_on_warn(self, printer):
        # Test the on_warn method to ensure it prints the indented text
        warning_message = "This is a warning message."
        expected_output = textwrap.indent(warning_message, "*** ") + "\n"
        printer.on_warn(warning_message)
        assert printer._out.getvalue() == expected_output

# Run the tests with pytest
# Note: The actual test run command is not included as per the instructions.
