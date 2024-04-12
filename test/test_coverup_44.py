# file src/chatdbg/util/log.py:86-89
# lines [87, 88, 89]
# branches ['87->88', '87->89']

import pytest
from chatdbg.util.log import ChatDBGLog
from unittest.mock import MagicMock

# Assuming BaseAssistantListener is part of a module named 'chatdbg.util.base'
# and that '_dump' and '_make_log' are methods within ChatDBGLog that can be mocked.
# If these assumptions are incorrect, please adjust the import paths and class names accordingly.

@pytest.fixture
def chat_dbg_log():
    log = ChatDBGLog(log_filename='dummy.log', config={})
    log._log = MagicMock()
    log._dump = MagicMock()
    log._make_log = MagicMock(return_value='new_log')
    return log

def test_on_end_dialog_with_log_not_none(chat_dbg_log):
    # Precondition: _log is not None
    assert chat_dbg_log._log is not None

    # Call the method under test
    chat_dbg_log.on_end_dialog()

    # Postconditions: _dump was called and _log was replaced
    chat_dbg_log._dump.assert_called_once()
    assert chat_dbg_log._log == 'new_log'

def test_on_end_dialog_with_log_none(chat_dbg_log, monkeypatch):
    # Precondition: _log is None
    monkeypatch.setattr(chat_dbg_log, '_log', None)

    # Call the method under test
    chat_dbg_log.on_end_dialog()

    # Postconditions: _dump was not called and _log was replaced
    chat_dbg_log._dump.assert_not_called()
    assert chat_dbg_log._log == 'new_log'
