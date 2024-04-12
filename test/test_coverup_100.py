# file src/chatdbg/native_util/dbg_dialog.py:230-231
# lines [230, 231]
# branches []

import pytest
from unittest.mock import MagicMock

# Assuming DBGDialog is in a module at the provided path
# Adjust the import path according to the actual module structure
from chatdbg.native_util.dbg_dialog import DBGDialog

# Mocking the DBGDialog __init__ to not require any arguments for simplicity
@pytest.fixture
def mock_dbg_dialog(monkeypatch):
    monkeypatch.setattr(DBGDialog, "__init__", lambda x: None)
    return DBGDialog()

# Test function for the `warn` method
def test_warn(capsys, mock_dbg_dialog):
    test_message = "Warning message"
    
    # Call the method we want to test
    mock_dbg_dialog.warn(test_message)
    
    # Capture the output
    captured = capsys.readouterr()
    
    # Assert the message has been printed
    assert captured.out.strip() == test_message

# Mocking print to ensure there is no actual print pollution
def test_warn_with_mock(monkeypatch, mock_dbg_dialog):
    mock_print = MagicMock()
    monkeypatch.setattr("builtins.print", mock_print)
    test_message = "Another warning message"
    
    # Call the method we want to test
    mock_dbg_dialog.warn(test_message)
    
    # Assert that print was called with the correct message
    mock_print.assert_called_once_with(test_message)
