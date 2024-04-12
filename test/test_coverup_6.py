# file src/chatdbg/util/trim.py:41-52
# lines [41, 42, 43, 44, 45, 46, 48, 49, 51, 52]
# branches ['44->45', '44->52', '45->46', '45->51']

import pytest
from chatdbg.util.trim import _extract
from unittest.mock import patch

@pytest.fixture
def mock_sandwich_tokens():
    with patch("chatdbg.util.trim.sandwich_tokens") as mock:
        mock.return_value = "sandwiched content"
        yield mock

@pytest.fixture
def mock_litellm_encode():
    with patch("chatdbg.util.trim.litellm.encode") as mock:
        mock.return_value = [1, 2, 3]
        yield mock

def test_extract_with_tool_call_ids(mock_sandwich_tokens, mock_litellm_encode):
    messages = [
        {"tool_call_id": 1, "content": "content1"},
        {"tool_call_id": 2, "content": "content2"},
        {"content": "content3"}
    ]
    model = "model"
    tool_call_ids = [1, 2]

    tools, other = _extract(messages, model, tool_call_ids)

    assert mock_sandwich_tokens.called
    assert len(tools) == 2
    assert len(other) == 1
    assert tools[0]["content"] == "sandwiched content"
    assert tools[1]["content"] == "sandwiched content"
    assert other[0]["content"] == "content3"

def test_extract_without_tool_call_ids(mock_sandwich_tokens, mock_litellm_encode):
    messages = [
        {"tool_call_id": 1, "content": "content1"},
        {"tool_call_id": 2, "content": "content2"},
        {"content": "content3"}
    ]
    model = "model"
    tool_call_ids = [3]

    tools, other = _extract(messages, model, tool_call_ids)

    assert not mock_sandwich_tokens.called
    assert len(tools) == 0
    assert len(other) == 3
    assert other[0]["content"] == "content1"
    assert other[1]["content"] == "content2"
    assert other[2]["content"] == "content3"
