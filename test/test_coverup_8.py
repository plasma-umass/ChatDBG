# file src/chatdbg/assistant/listeners.py:89-103
# lines [89, 90, 91, 93, 94, 96, 97, 99, 100, 102, 103]
# branches []

import sys
from io import StringIO
import pytest
from chatdbg.assistant.listeners import StreamingPrinter

# Test for on_begin_stream
def test_on_begin_stream(capsys):
    printer = StreamingPrinter()
    printer.on_begin_stream()
    captured = capsys.readouterr()
    assert captured.out == "\n"

# Test for on_stream_delta
def test_on_stream_delta(capsys):
    stream_output = StringIO()
    printer = StreamingPrinter(out=stream_output)
    test_text = "Test stream delta"
    printer.on_stream_delta(test_text)
    assert stream_output.getvalue() == test_text

# Test for on_end_stream
def test_on_end_stream(capsys):
    printer = StreamingPrinter()
    printer.on_end_stream()
    captured = capsys.readouterr()
    assert captured.out == "\n"

# Test for on_response
def test_on_response(capsys):
    printer = StreamingPrinter()
    response_text = "Test response"
    printer.on_response(response_text)
    captured = capsys.readouterr()
    # on_response is a pass-through function, so no output is expected
    assert captured.out == ""
