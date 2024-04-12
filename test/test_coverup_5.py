# file src/chatdbg/util/log.py:29-50
# lines [29, 30, 31, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 46, 47, 48]
# branches []

import pytest
import sys
import uuid
from datetime import datetime
from io import StringIO
from unittest.mock import MagicMock

# Assuming BaseAssistantListener is part of the module we're testing
from chatdbg.util.log import ChatDBGLog

@pytest.fixture
def chat_dbg_log(monkeypatch):
    monkeypatch.setattr('sys.argv', ['test', 'args'])
    log = ChatDBGLog(log_filename='test_log.log', config={'test': 'config'})
    log._stdout_wrapper = StringIO()
    log._stderr_wrapper = StringIO()
    return log

@pytest.fixture
def chat_dbg_log_without_wrappers(monkeypatch):
    monkeypatch.setattr('sys.argv', ['test', 'args'])
    log = ChatDBGLog(log_filename='test_log.log', config={'test': 'config'})
    log._stdout_wrapper = None
    log._stderr_wrapper = None
    return log

def test_make_log_with_wrappers(chat_dbg_log, monkeypatch):
    # Mocking uuid.uuid4
    monkeypatch.setattr(uuid, "uuid4", lambda: "1234")

    # Write to the wrappers to test if the output is captured
    chat_dbg_log._stdout_wrapper.write("stdout test")
    chat_dbg_log._stderr_wrapper.write("stderr test")

    log = chat_dbg_log._make_log()

    assert log["meta"]["command_line"] == "test args"
    assert log["meta"]["uid"] == "1234"
    assert log["meta"]["config"] == {'test': 'config'}
    assert log["stdout"] == "stdout test"
    assert log["stderr"] == "stderr test"

def test_make_log_without_wrappers(chat_dbg_log_without_wrappers, monkeypatch):
    # Mocking uuid.uuid4
    monkeypatch.setattr(uuid, "uuid4", lambda: "1234")

    log = chat_dbg_log_without_wrappers._make_log()

    assert log["meta"]["command_line"] == "test args"
    assert log["meta"]["uid"] == "1234"
    assert log["meta"]["config"] == {'test': 'config'}
    assert log["stdout"] is None
    assert log["stderr"] is None
