# file src/chatdbg/native_util/dbg_dialog.py:132-149
# lines [132, 149]
# branches []

import pytest
from unittest.mock import MagicMock, create_autospec

# Assuming DBGDialog is part of a module named dbg_dialog under chatdbg/native_util
from chatdbg.native_util.dbg_dialog import DBGDialog

# Test function to cover llm_debug method
def test_llm_debug():
    # Mock dependencies
    prompt_mock = MagicMock()

    # Create an instance of DBGDialog with a mocked prompt
    dbg_dialog_instance = DBGDialog(prompt_mock)
    dbg_dialog_instance._run_one_command = create_autospec(dbg_dialog_instance._run_one_command, return_value='mocked response')

    # Call llm_debug with a test command
    command = 'test_command'
    result = dbg_dialog_instance.llm_debug(command)

    # Assert the return value is correct
    assert result == (command, 'mocked response')

    # Assert that _run_one_command was called with the correct arguments
    dbg_dialog_instance._run_one_command.assert_called_once_with(command)
