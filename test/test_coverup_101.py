# file src/chatdbg/util/markdown.py:157-158
# lines [157, 158]
# branches []

# test_chatdbg_markdown.py

import pytest
from chatdbg.util.markdown import ChatDBGMarkdownPrinter
from unittest.mock import Mock

@pytest.fixture
def chatdbg_markdown_printer():
    out = Mock()
    debugger_prompt = Mock()
    chat_prefix = Mock()
    width = Mock()
    return ChatDBGMarkdownPrinter(out, debugger_prompt, chat_prefix, width)

def test_on_begin_stream(chatdbg_markdown_printer):
    # Ensure that the initial state of _streamed is empty
    assert chatdbg_markdown_printer._streamed == ""
    
    # Simulate the on_begin_stream event
    chatdbg_markdown_printer.on_begin_stream()
    
    # Verify that _streamed is still an empty string after the event
    assert chatdbg_markdown_printer._streamed == ""

# Ensure cleanup is not needed because the test does not modify any shared state
