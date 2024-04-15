# file src/chatdbg/native_util/dbg_dialog.py:105-110
# lines [105, 110]
# branches []

import pytest
from unittest.mock import Mock
from chatdbg.native_util.dbg_dialog import DBGDialog

# Correcting the test to mock the 'prompt' argument in the DBGDialog constructor
@pytest.fixture
def mock_dbg_dialog():
    prompt_mock = Mock()
    return DBGDialog(prompt_mock)

def test_prompt_stack(mock_dbg_dialog):
    assert mock_dbg_dialog._prompt_stack() is None
