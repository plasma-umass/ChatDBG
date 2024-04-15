# file src/chatdbg/util/log.py:91-99
# lines [92, 93, 94, 95, 96, 97, 98]
# branches []

import pytest
from chatdbg.util.log import ChatDBGLog
from unittest.mock import Mock

@pytest.fixture
def chat_dbg_log(monkeypatch):
    log_mock = Mock()
    chat_dbg_log_instance = ChatDBGLog(log_filename='dummy.log', config={})
    monkeypatch.setattr(chat_dbg_log_instance, "_log", log_mock)
    monkeypatch.setattr(chat_dbg_log_instance, "_current_chat", None)
    return chat_dbg_log_instance

def test_on_begin_query(chat_dbg_log):
    prompt = "Test prompt"
    extra = "Test extra"
    chat_dbg_log.on_begin_query(prompt, extra)
    assert chat_dbg_log._log is not None
    assert chat_dbg_log._current_chat is not None
    assert chat_dbg_log._current_chat["input"] == extra
    assert chat_dbg_log._current_chat["prompt"] == prompt
    assert chat_dbg_log._current_chat["output"] == {"type": "chat", "outputs": []}
