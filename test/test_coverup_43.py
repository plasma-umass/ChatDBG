# file src/chatdbg/assistant/listeners.py:52-86
# lines [63, 69, 72, 75]
# branches []

import sys
import pytest
from io import StringIO
from chatdbg.assistant.listeners import Printer

@pytest.fixture
def mock_stdout(monkeypatch):
    new_out = StringIO()
    monkeypatch.setattr(sys, 'stdout', new_out)
    return new_out

def test_on_begin_stream(mock_stdout):
    printer = Printer(out=mock_stdout)
    printer.on_begin_stream()
    assert mock_stdout.getvalue() == ""

def test_on_end_stream(mock_stdout):
    printer = Printer(out=mock_stdout)
    printer.on_end_stream()
    assert mock_stdout.getvalue() == ""

def test_on_begin_query(mock_stdout):
    printer = Printer(out=mock_stdout)
    printer.on_begin_query(prompt="", user_text="")
    assert mock_stdout.getvalue() == ""

def test_on_end_query(mock_stdout):
    printer = Printer(out=mock_stdout)
    printer.on_end_query(stats={})
    assert mock_stdout.getvalue() == ""
