# file src/chatdbg/util/printer.py:66-72
# lines [67, 68, 69, 70, 72]
# branches ['67->exit', '67->68']

import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import MagicMock

# Assuming word_wrap_except_code_blocks is a function that needs to be mocked
# and _print is a method of ChatDBGPrinter that prints the text

@pytest.fixture
def mock_printer(monkeypatch):
    mock_out = MagicMock()
    mock_word_wrap = MagicMock(side_effect=lambda text, width: text)
    monkeypatch.setattr("chatdbg.util.printer.word_wrap_except_code_blocks", mock_word_wrap)
    
    printer = ChatDBGPrinter(out=mock_out, debugger_prompt='>', chat_prefix='[chatdbg] ', width=80)
    printer._at_start = True
    mock_print = MagicMock()
    monkeypatch.setattr(printer, "_print", mock_print)
    return printer, mock_print

def test_on_response_with_non_none_text(mock_printer):
    printer, mock_print = mock_printer
    test_text = "Hello, World!"
    printer.on_response(test_text)
    mock_print.assert_called_once_with("(Message) " + test_text)

def test_on_response_with_none_text(mock_printer):
    printer, mock_print = mock_printer
    printer.on_response(None)
    mock_print.assert_not_called()
