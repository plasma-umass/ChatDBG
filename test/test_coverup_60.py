# file src/chatdbg/util/log.py:128-133
# lines [129, 130, 131, 132, 133]
# branches []

import pytest
from chatdbg.util.log import ChatDBGLog
from unittest.mock import MagicMock

@pytest.fixture
def chat_dbg_log():
    log = ChatDBGLog(log_filename='dummy.log', config={})
    log._log = MagicMock()
    log._current_chat = {"output": {"outputs": []}}
    return log

def test_on_response(chat_dbg_log):
    test_text = "Test response text"
    chat_dbg_log.on_response(test_text)
    assert chat_dbg_log._current_chat["output"]["outputs"] == [{"type": "text", "output": test_text}]

def test_on_response_with_word_wrap(chat_dbg_log, monkeypatch):
    test_text = "Test response text that should be wrapped"
    expected_wrapped_text = "Wrapped text"

    def mock_word_wrap_except_code_blocks(text):
        return expected_wrapped_text

    monkeypatch.setattr('chatdbg.util.log.word_wrap_except_code_blocks', mock_word_wrap_except_code_blocks)
    chat_dbg_log.on_response(test_text)
    assert chat_dbg_log._current_chat["output"]["outputs"] == [{"type": "text", "output": expected_wrapped_text}]
