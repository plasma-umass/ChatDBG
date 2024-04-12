# file src/chatdbg/util/trim.py:51-61
# lines [52, 53, 54, 55, 56, 57, 59, 60, 61]
# branches ['52->53', '52->54', '55->56', '55->59']

import pytest
from unittest.mock import MagicMock
from chatdbg.util.trim import _chunkify

@pytest.fixture
def sandwich_tokens_mock():
    mock = MagicMock(return_value='sandwiched_content')
    return mock

@pytest.fixture
def extract_mock():
    mock = MagicMock(return_value=([], []))
    return mock

def test_chunkify_empty_list():
    assert _chunkify([], None) == []

def test_chunkify_without_tool_calls(sandwich_tokens_mock, monkeypatch):
    messages = [{"content": "test"}]
    model = "model"
    monkeypatch.setattr('chatdbg.util.trim.sandwich_tokens', sandwich_tokens_mock)
    result = _chunkify(messages, model)
    sandwich_tokens_mock.assert_called_once_with("test", model, 1024, 0)
    assert result == [([{"content": "sandwiched_content"}], False)]

def test_chunkify_with_tool_calls(extract_mock, monkeypatch):
    messages = [{"tool_calls": [{"id": "1"}], "content": "test"}]
    model = "model"
    monkeypatch.setattr('chatdbg.util.trim._extract', extract_mock)
    result = _chunkify(messages, model)
    extract_mock.assert_called_once_with([], model, ["1"])
    assert result == [([{"tool_calls": [{"id": "1"}], "content": "test"}], False)]

# Run the tests
pytest.main()
