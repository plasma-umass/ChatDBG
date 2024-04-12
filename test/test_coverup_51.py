# file src/chatdbg/util/printer.py:43-47
# lines [44, 45, 47]
# branches []

import pytest
from unittest.mock import Mock

# Assuming ChatDBGPrinter is available from the correct module
from chatdbg.util.printer import ChatDBGPrinter

# Mocking StreamingTextWrapper to avoid side effects
class MockStreamingTextWrapper:
    def __init__(self, chat_prefix, width):
        self.chat_prefix = chat_prefix
        self.width = width

# Test function to cover lines 44-47
def test_on_begin_stream(monkeypatch):
    # Mock StreamingTextWrapper to avoid side effects
    monkeypatch.setattr("chatdbg.util.printer.StreamingTextWrapper", MockStreamingTextWrapper)

    # Mock dependencies for ChatDBGPrinter
    mock_out = Mock()
    debugger_prompt = "debug>"
    chat_prefix = "TestPrefix"
    width = 80

    # Create an instance of ChatDBGPrinter with mocked dependencies
    printer = ChatDBGPrinter(mock_out, debugger_prompt, chat_prefix, width)

    # Call on_begin_stream to execute lines 44-47
    printer.on_begin_stream()

    # Assertions to verify postconditions
    assert isinstance(printer._stream_wrapper, MockStreamingTextWrapper)
    assert printer._stream_wrapper.chat_prefix == chat_prefix
    assert printer._stream_wrapper.width == width
    assert printer._at_start is True
