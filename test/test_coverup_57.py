# file src/chatdbg/util/log.py:109-123
# lines [110, 111, 112, 113, 114, 117, 118, 119, 120, 121]
# branches ['112->113', '112->117']

import pytest
from chatdbg.util.log import ChatDBGLog

@pytest.fixture
def chat_dbg_log():
    log = ChatDBGLog(log_filename='test.log', config={})
    log._log = {"steps": []}
    return log

def test_post_with_current_chat(chat_dbg_log, monkeypatch):
    monkeypatch.setattr(chat_dbg_log, '_current_chat', {"output": {"outputs": []}})
    chat_dbg_log._post("Test message", "INFO")
    assert chat_dbg_log._current_chat["output"]["outputs"] == [
        {"type": "text", "output": "*** INFO: Test message"}
    ]

def test_post_without_current_chat(chat_dbg_log):
    chat_dbg_log._post("Test message", "INFO")
    assert chat_dbg_log._log["steps"] == [
        {
            "type": "call",
            "input": "*** INFO",
            "output": {"type": "text", "output": "Test message"},
        }
    ]
