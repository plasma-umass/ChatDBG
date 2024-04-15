# file src/chatdbg/util/markdown.py:141-144
# lines [141, 142, 143]
# branches []

import pytest
from chatdbg.util.markdown import ChatDBGMarkdownPrinter
from unittest.mock import MagicMock

@pytest.fixture
def markdown_printer(monkeypatch):
    monkeypatch.setattr(ChatDBGMarkdownPrinter, "__init__", lambda self: None)
    printer = ChatDBGMarkdownPrinter()
    printer._width = 80
    printer._left_indent = 0
    printer._print = MagicMock()
    printer._wrap_in_panel = lambda x: x
    printer._wrap_and_fill_and_indent = lambda text, prefix, style: f"{prefix}{text}{prefix}"
    return printer

def test_message(markdown_printer):
    test_text = "Test Message"
    test_style = "test-style"
    markdown_printer._message(test_text, test_style)
    markdown_printer._print.assert_called_once()
    args, kwargs = markdown_printer._print.call_args
    expected_output = "*** Test Message ***"
    assert expected_output in args[0]
