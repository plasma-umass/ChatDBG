# file src/chatdbg/util/markdown.py:122-123
# lines [122, 123]
# branches []

import pytest
from chatdbg.util.markdown import ChatDBGMarkdownPrinter
from unittest.mock import Mock, MagicMock

@pytest.fixture
def markdown_printer():
    printer = ChatDBGMarkdownPrinter(out=MagicMock(), debugger_prompt=MagicMock(), chat_prefix=MagicMock(), width=MagicMock())
    printer._console = Mock()
    return printer

def test_print_with_default_end(markdown_printer):
    renderable = "Test message"
    markdown_printer._print(renderable)
    markdown_printer._console.print.assert_called_once_with(renderable, end="")

def test_print_with_custom_end(markdown_printer):
    renderable = "Test message"
    custom_end = "\n"
    markdown_printer._print(renderable, end=custom_end)
    markdown_printer._console.print.assert_called_once_with(renderable, end=custom_end)
