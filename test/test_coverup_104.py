# file src/chatdbg/native_util/dbg_dialog.py:112-113
# lines [112, 113]
# branches []

import pytest
from unittest.mock import MagicMock
from chatdbg.native_util.dbg_dialog import DBGDialog

# Test function to check if _prompt_history method returns the correct history
def test_prompt_history(monkeypatch):
    # Mocking the __init__ method to not require the 'prompt' parameter
    monkeypatch.setattr(DBGDialog, "__init__", lambda self: None)
    
    dialog = DBGDialog()
    dialog._history = ["User: Hi", "Bot: Hello"]
    history = dialog._prompt_history()
    assert history == "['User: Hi', 'Bot: Hello']"

# Test function to check if _prompt_history handles an empty history
def test_prompt_history_empty(monkeypatch):
    # Mocking the __init__ method to not require the 'prompt' parameter
    monkeypatch.setattr(DBGDialog, "__init__", lambda self: None)
    
    dialog = DBGDialog()
    dialog._history = []
    history = dialog._prompt_history()
    assert history == "[]"
