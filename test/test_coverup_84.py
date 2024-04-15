# file src/chatdbg/native_util/dbg_dialog.py:36-39
# lines [36, 37, 38, 39]
# branches []

import pytest
from unittest.mock import MagicMock, call

# Assuming the DBGDialog class is part of a module named `chatdbg.native_util.dbg_dialog`
# and that it requires a 'prompt' argument for initialization
from chatdbg.native_util.dbg_dialog import DBGDialog

# Test function to execute the query_and_print method
def test_query_and_print(capsys, monkeypatch):
    # Create an instance of DBGDialog with a mock prompt
    dialog = DBGDialog(prompt=MagicMock())

    # Mock the build_prompt method to return a test prompt
    test_prompt = "Test prompt"
    monkeypatch.setattr(dialog, 'build_prompt', MagicMock(return_value=test_prompt))
   
    # Mock the query method of the assistant object to return a test message
    assistant_mock = MagicMock()
    assistant_mock.query.return_value = {"message": "Test message"}

    # Mock the _history attribute to provide a clear method
    history_mock = MagicMock()
    monkeypatch.setattr(dialog, '_history', history_mock)

    # Call the method under test
    dialog.query_and_print(assistant_mock, "user text", False)

    # Capture the output
    captured = capsys.readouterr()

    # Assert that build_prompt is called correctly
    dialog.build_prompt.assert_called_once_with("user text", False)

    # Assert that the assistant's query method is called correctly
    assistant_mock.query.assert_called_once_with(test_prompt, "user text")

    # Assert that the _history's clear method is called
    history_mock.clear.assert_called_once()

    # Assert that the correct message is printed
    assert captured.out == "Test message\n"
