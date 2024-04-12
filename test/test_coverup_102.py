# file src/chatdbg/util/jupyter.py:52-53
# lines [52, 53]
# branches []

import pytest
from chatdbg.util.jupyter import ChatDBGJupyterPrinter

# The test class for ChatDBGJupyterPrinter
class TestChatDBGJupyterPrinter:
    
    # Test the on_begin_stream method
    def test_on_begin_stream(self, monkeypatch):
        # Mock the __init__ method to do nothing
        monkeypatch.setattr(ChatDBGJupyterPrinter, '__init__', lambda *args, **kwargs: None)
        
        printer = ChatDBGJupyterPrinter()
        # Initialize the _streamed attribute since the original __init__ is bypassed
        printer._streamed = ""  
        assert printer._streamed == ""  # Assert initial state
        
        # Call the method we want to test
        printer.on_begin_stream()
        
        # Assert that the method does what we expect
        assert printer._streamed == ""

@pytest.fixture
def chat_dbg_jupyter_printer(monkeypatch):
    # Apply the monkeypatch for all tests in this module
    monkeypatch.setattr(ChatDBGJupyterPrinter, '__init__', lambda *args, **kwargs: None)
    printer = ChatDBGJupyterPrinter()
    printer._streamed = ""  # Initialize the attribute since __init__ is mocked
    return printer

# Pytest code to run the test using fixture
def test_on_begin_stream(chat_dbg_jupyter_printer):
    # Assert initial state
    assert chat_dbg_jupyter_printer._streamed == ""
    
    # Call the method we want to test
    chat_dbg_jupyter_printer.on_begin_stream()
    
    # Assert postcondition
    assert chat_dbg_jupyter_printer._streamed == ""
