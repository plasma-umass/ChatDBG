# file src/chatdbg/chatdbg_pdb.py:31-38
# lines [31, 33, 34, 36, 37, 38]
# branches []

import pytest
from unittest.mock import MagicMock
from types import SimpleNamespace

# Assuming chatdbg.chatdbg_pdb and chatdbg.util.config are accessible from the test file

# Create mock for ipython
@pytest.fixture
def mock_ipython(monkeypatch):
    ipython_mock = MagicMock()
    ipython_mock.config = {}
    return ipython_mock

# Create a test to cover load_ipython_extension function
def test_load_ipython_extension(mock_ipython, monkeypatch):
    # Mock the ChatDBG and ChatDBGConfig classes
    monkeypatch.setattr('chatdbg.chatdbg_pdb.ChatDBG', MagicMock())
    ChatDBGConfigMock = MagicMock()
    monkeypatch.setattr('chatdbg.util.config.ChatDBGConfig', ChatDBGConfigMock)
    
    # Import the function to test
    from chatdbg.chatdbg_pdb import load_ipython_extension
    
    # Call the function with the mock
    load_ipython_extension(mock_ipython)
    
    # Assert that the ChatDBG class has been set as the debugger_cls
    assert mock_ipython.InteractiveTB.debugger_cls is not None
    
    # Assert that the ChatDBGConfig has been instantiated with the config
    ChatDBGConfigMock.assert_called_with(config=mock_ipython.config)
    
    # Cleanup by removing any added attributes to the mock
    del mock_ipython.InteractiveTB.debugger_cls

# Run the test
def test_load_ipython_extension_cleanup(mock_ipython, monkeypatch):
    # Mock the print function to avoid actual print during test
    monkeypatch.setattr('builtins.print', MagicMock())
    
    # Re-run the test to ensure cleanup is effective
    test_load_ipython_extension(mock_ipython, monkeypatch)
    
    # Assert cleanup, there should be no debugger_cls set after cleanup
    assert not hasattr(mock_ipython.InteractiveTB, 'debugger_cls')
