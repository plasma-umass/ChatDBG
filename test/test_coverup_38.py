# file src/chatdbg/util/printer.py:49-60
# lines [50, 51, 52, 53, 54, 55, 56, 58, 59]
# branches ['50->51', '50->58']

import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import MagicMock

class MockStreamWrapper:
    def __init__(self):
        self.text = ""

    def append(self, text, new_line=True):
        self.text += text
        return text

@pytest.fixture
def mock_stream_wrapper():
    return MockStreamWrapper()

@pytest.fixture
def mock_file(monkeypatch):
    mock_file = MagicMock()
    monkeypatch.setattr("builtins.print", mock_file)
    return mock_file

@pytest.fixture
def printer(mock_stream_wrapper, mock_file):
    printer = ChatDBGPrinter(out=mock_file, debugger_prompt='>', chat_prefix='-', width=80)
    printer._stream_wrapper = mock_stream_wrapper
    printer._at_start = True
    return printer

def test_on_stream_delta_at_start(printer, mock_file):
    printer.on_stream_delta("Hello, world!")
    mock_file.assert_any_call("\n(Message) ", end="", flush=True, file=mock_file)
    mock_file.assert_any_call("Hello, world!", end="", flush=True, file=mock_file)
    assert not printer._at_start

def test_on_stream_delta_not_at_start(printer, mock_file):
    printer._at_start = False
    printer.on_stream_delta("Hello again!")
    mock_file.assert_called_once_with("Hello again!", end="", flush=True, file=mock_file)
