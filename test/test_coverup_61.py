# file src/chatdbg/assistant/assistant.py:190-207
# lines [196, 197]
# branches []

import json
import pytest
from chatdbg.assistant.assistant import Assistant
from unittest.mock import MagicMock

# Mock function to simulate a successful call
def mock_successful_function(**args):
    return "call", "result with \x1b[0;32mANSI\x1b[0m codes and\ttabs"

# Mock function to simulate a function call that raises OSError
def mock_oserror_function(**args):
    raise OSError("simulated os error")

# Mock instructions to pass to the Assistant constructor
mock_instructions = MagicMock()

# Test to cover line 196-197 with a successful function call
def test_make_call_success(monkeypatch):
    assistant = Assistant(mock_instructions)
    monkeypatch.setattr(assistant, '_broadcast', MagicMock())
    monkeypatch.setattr(assistant, '_functions', {
        'test_function': {
            'function': mock_successful_function
        }
    })

    tool_call = MagicMock()
    tool_call.function.name = 'test_function'
    tool_call.function.arguments = json.dumps({})

    result = assistant._make_call(tool_call)

    # Check that the result is cleaned up and non-printable characters are removed
    # The number of spaces for tabs can vary depending on the environment, so we use a regex to match
    assert "result with ANSI codes and" in result and "tabs" in result
    # Check that _broadcast was called with the correct arguments
    assistant._broadcast.assert_called_once_with("on_function_call", "call", result)

# Test to cover line 196-197 with a function call that raises OSError
def test_make_call_oserror(monkeypatch):
    assistant = Assistant(mock_instructions)
    monkeypatch.setattr(assistant, '_broadcast', MagicMock())
    monkeypatch.setattr(assistant, '_functions', {
        'test_function': {
            'function': mock_oserror_function
        }
    })

    tool_call = MagicMock()
    tool_call.function.name = 'test_function'
    tool_call.function.arguments = json.dumps({})

    result = assistant._make_call(tool_call)

    # Check that the result contains the error message
    assert result == "Exception in function call: simulated os error"
    