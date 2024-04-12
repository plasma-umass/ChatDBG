# file src/chatdbg/assistant/listeners.py:52-86
# lines [52, 53, 54, 56, 57, 59, 60, 62, 63, 65, 66, 68, 69, 71, 72, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86]
# branches ['78->exit', '78->79', '82->83', '82->85']

import sys
import textwrap
from io import StringIO
import pytest
from chatdbg.assistant.listeners import Printer

@pytest.fixture
def mock_stdout():
    new_out = StringIO()
    return new_out

def test_on_warn(mock_stdout):
    printer = Printer(out=mock_stdout)
    warning_message = "Warning message"
    printer.on_warn(warning_message)
    assert mock_stdout.getvalue() == textwrap.indent(warning_message, "*** ") + "\n"

def test_on_error(mock_stdout):
    printer = Printer(out=mock_stdout)
    error_message = "Error message"
    printer.on_error(error_message)
    assert mock_stdout.getvalue() == textwrap.indent(error_message, "*** ") + "\n"

def test_on_stream_delta(mock_stdout):
    printer = Printer(out=mock_stdout)
    stream_delta = "Stream delta"
    printer.on_stream_delta(stream_delta)
    assert mock_stdout.getvalue() == stream_delta

def test_on_response_with_text(mock_stdout):
    printer = Printer(out=mock_stdout)
    response_text = "Response text"
    printer.on_response(response_text)
    assert mock_stdout.getvalue() == response_text + "\n"

def test_on_response_with_none(mock_stdout):
    printer = Printer(out=mock_stdout)
    printer.on_response(None)
    assert mock_stdout.getvalue() == ""

def test_on_function_call_with_result(mock_stdout):
    printer = Printer(out=mock_stdout)
    function_call = "function_call()"
    result = "Result"
    printer.on_function_call(function_call, result)
    assert mock_stdout.getvalue() == f"{function_call}\n{result}\n"

def test_on_function_call_without_result(mock_stdout):
    printer = Printer(out=mock_stdout)
    function_call = "function_call()"
    printer.on_function_call(function_call, "")
    assert mock_stdout.getvalue() == f"{function_call}\n"
