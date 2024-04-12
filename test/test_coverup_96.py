# file src/chatdbg/native_util/dbg_dialog.py:102-103
# lines [102, 103]
# branches []

import pytest
from unittest.mock import MagicMock

from chatdbg.native_util.dbg_dialog import DBGDialog

# Since DBGDialog seems to require 'prompt' argument in constructor, let's mock it
class MockDBGDialog(DBGDialog):
    def __init__(self):
        super().__init__(prompt=MagicMock())

    def _initial_prompt_input(self):
        # Override to prevent asking for real user input during tests
        return "mock input"

# Test function to execute the _initial_prompt_input method
def test_initial_prompt_input():
    dialog = MockDBGDialog()
    assert dialog._initial_prompt_input() == "mock input"

# Test the original behavior
def test_initial_prompt_input_original():
    dialog = DBGDialog(prompt=MagicMock())
    assert dialog._initial_prompt_input() is None
