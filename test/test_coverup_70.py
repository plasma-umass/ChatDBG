# file src/chatdbg/util/markdown.py:176-185
# lines [176, 177, 178, 179, 181, 182, 184, 185]
# branches []

import pytest
from unittest.mock import MagicMock

# Assuming the rest of the required classes and methods are imported or defined correctly somewhere else
from chatdbg.util.markdown import ChatDBGMarkdownPrinter, escape, fill_to_width

@pytest.fixture
def mock_printer(monkeypatch):
    mock_out = MagicMock()
    debugger_prompt = ">>>"
    chat_prefix = "Prefix"
    width = 70
    printer = ChatDBGMarkdownPrinter(mock_out, debugger_prompt, chat_prefix, width)
    printer._print = MagicMock()
    printer._wrap_in_panel = MagicMock(side_effect=lambda x: f"Panel({x})")
    monkeypatch.setattr('chatdbg.util.markdown.fill_to_width', fill_to_width)
    return printer

def test_on_function_call(mock_printer):
    call = "func_call()"
    result = "Function result\n"
    mock_printer.on_function_call(call, result)
    prefix = mock_printer._chat_prefix
    line = fill_to_width(f"\n{prefix}{mock_printer._debugger_prompt}{call}", mock_printer._width)
    expected_entry = f"[command]{escape(line)}[/]\n"
    expected_entry += mock_printer._wrap_and_fill_and_indent(
        result.rstrip() + "\n", prefix, "result"
    )
    expected_panel = f"Panel({expected_entry})"
    mock_printer._print.assert_called_once_with(expected_panel, end="")
