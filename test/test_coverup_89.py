# file src/chatdbg/util/markdown.py:166-168
# lines [166, 167, 168]
# branches ['167->exit', '167->168']

import pytest
from unittest.mock import MagicMock

# Assuming BaseAssistantListener is part of the chatdbg.util.markdown module
# and that it contains an __init__ that initializes self._streamed and self._live,
# along with the required arguments 'out', 'debugger_prompt', 'chat_prefix', and 'width'
from chatdbg.util.markdown import ChatDBGMarkdownPrinter

# Since the class is inheriting from BaseAssistantListener in the actual module,
# we don't need to redefine the base class, we can directly patch the __init__ method.

@pytest.fixture
def markdown_printer(monkeypatch):
    # Mocking the __init__ method to avoid needing the required arguments
    # and to add the _live attribute
    with monkeypatch.context() as m:
        m.setattr(ChatDBGMarkdownPrinter, "__init__", lambda self: None)
        printer = ChatDBGMarkdownPrinter()
        printer._streamed = ""
        printer._live = MagicMock()
        yield printer

def test_on_end_stream_with_streamed_content(markdown_printer):
    # Set _streamed to some content to trigger the if condition
    markdown_printer._streamed = "some content"
    markdown_printer.on_end_stream()
    # Assert that _live.stop() was called
    assert markdown_printer._live.stop.called

def test_on_end_stream_without_streamed_content(markdown_printer):
    # _streamed is set to "" by default, so the if condition should not trigger
    markdown_printer.on_end_stream()
    # Assert that _live.stop() was not called
    assert not markdown_printer._live.stop.called
