# file src/chatdbg/native_util/dbg_dialog.py:95-97
# lines [95, 97]
# branches []

import pytest
from unittest.mock import Mock
from chatdbg.native_util.dbg_dialog import DBGDialog

class MockPrompt:
    def __init__(self):
        pass

@pytest.fixture
def mock_prompt():
    return MockPrompt()

@pytest.fixture
def dbg_dialog(mock_prompt):
    return DBGDialog(prompt=mock_prompt)

def test_initial_prompt_error_details(dbg_dialog):
    assert dbg_dialog._initial_prompt_error_details() is None
