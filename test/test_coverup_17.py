# file src/chatdbg/util/printer.py:9-20
# lines [9, 10, 11, 12, 13, 14, 15, 17, 20]
# branches []

import os
import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import Mock

# Mocking os.get_terminal_size to raise an OSError
def test_get_terminal_size_failure(monkeypatch):
    def mock_get_terminal_size():
        raise OSError("get_terminal_size failed")

    monkeypatch.setattr(os, "get_terminal_size", mock_get_terminal_size)

    out = Mock()
    debugger_prompt = "debug>"
    chat_prefix = "chat>"
    width = 80

    printer = ChatDBGPrinter(out, debugger_prompt, chat_prefix, width)

    assert printer._width == width
    assert printer._debugger_prompt == debugger_prompt
    assert printer._chat_prefix == chat_prefix
    assert printer._at_start is True

# Mocking os.get_terminal_size to return a smaller width than provided
def test_get_terminal_size_smaller_width(monkeypatch):
    def mock_get_terminal_size():
        columns = 50
        return os.terminal_size((columns, 24))

    monkeypatch.setattr(os, "get_terminal_size", mock_get_terminal_size)

    out = Mock()
    debugger_prompt = "debug>"
    chat_prefix = "chat>"
    width = 80

    printer = ChatDBGPrinter(out, debugger_prompt, chat_prefix, width)

    expected_width = 50 - len(chat_prefix)
    assert printer._width == expected_width
    assert printer._debugger_prompt == debugger_prompt
    assert printer._chat_prefix == chat_prefix
    assert printer._at_start is True
