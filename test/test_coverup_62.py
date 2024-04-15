# file src/chatdbg/util/log.py:135-153
# lines [136, 137, 138, 139, 140, 141, 142, 143, 147, 148, 149, 150, 151]
# branches ['138->139', '138->147']

import pytest
from chatdbg.util.log import ChatDBGLog
from unittest.mock import MagicMock

# Mocking the BaseAssistantListener which ChatDBGLog might be inheriting from
class MockBaseAssistantListener:
    def __init__(self, log_filename, config):
        pass


# Applying the monkeypatch to replace BaseAssistantListener with the mocked class
@pytest.fixture(autouse=True)
def apply_monkeypatch(monkeypatch):
    monkeypatch.setattr(
        'chatdbg.util.log.BaseAssistantListener', MockBaseAssistantListener
    )


@pytest.fixture
def chatdbg_log():
    log = ChatDBGLog('dummy_log_filename', {})
    log._log = {'steps': []}
    log._current_chat = None
    return log


@pytest.fixture
def chatdbg_log_with_chat():
    log = ChatDBGLog('dummy_log_filename', {})
    log._log = {'steps': []}
    log._current_chat = {"output": {"outputs": []}}
    return log


def test_on_function_call_without_current_chat(chatdbg_log):
    call = "test_call"
    result = "test_result"
    chatdbg_log.on_function_call(call, result)
    
    assert chatdbg_log._log['steps'][-1] == {
        "type": "call",
        "input": call,
        "output": {"type": "text", "output": result},
    }


def test_on_function_call_with_current_chat(chatdbg_log_with_chat):
    call = "test_call"
    result = "test_result"
    chatdbg_log_with_chat.on_function_call(call, result)

    assert chatdbg_log_with_chat._current_chat["output"]["outputs"][-1] == {
        "type": "call",
        "input": call,
        "output": {"type": "text", "output": result},
    }
