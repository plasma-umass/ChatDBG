# file src/chatdbg/native_util/dbg_dialog.py:82-83
# lines [82, 83]
# branches []

import pytest
from chatdbg.native_util.dbg_dialog import DBGDialog

# Since the error indicates that the DBGDialog constructor requires a 'prompt' argument,
# we must modify the fixture to provide this argument. We will assume 'prompt' is a string.

@pytest.fixture
def dbg_dialog():
    return DBGDialog(prompt="dummy_prompt")

def test_get_frame_summaries(dbg_dialog):
    # Calling the method to ensure it is covered
    summaries = dbg_dialog._get_frame_summaries()
    # Since the method does not do anything, it should return None
    assert summaries is None

    # Calling with a specific max_entries to cover the default argument
    summaries_with_arg = dbg_dialog._get_frame_summaries(max_entries=10)
    assert summaries_with_arg is None
