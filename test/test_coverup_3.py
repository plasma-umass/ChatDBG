# file src/chatdbg/util/trim.py:10-26
# lines [10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 24, 25]
# branches ['13->14', '13->15', '16->17', '16->19']

import pytest
from chatdbg.util.trim import sandwich_tokens
from unittest.mock import MagicMock

# Mocking the litellm.encode and litellm.decode functions
@pytest.fixture
def mock_litellm(monkeypatch):
    encode_mock = MagicMock(return_value=list(range(2000)))
    decode_mock = MagicMock(side_effect=lambda model, tokens: 'decoded')
    monkeypatch.setattr('chatdbg.util.trim.litellm.encode', encode_mock)
    monkeypatch.setattr('chatdbg.util.trim.litellm.decode', decode_mock)
    return encode_mock, decode_mock

def test_sandwich_tokens_max_tokens_none(mock_litellm):
    text = "This is a test text."
    model = "test-model"
    result = sandwich_tokens(text, model, max_tokens=None)
    assert result == text

def test_sandwich_tokens_within_limit(mock_litellm):
    text = "This is a test text."
    model = "test-model"
    result = sandwich_tokens(text, model, max_tokens=2048)
    assert result == text

def test_sandwich_tokens_exceeds_limit(mock_litellm):
    text = "This is a test text."
    model = "test-model"
    result = sandwich_tokens(text, model, max_tokens=1024)
    assert result == "decoded [...] decoded"

def test_sandwich_tokens_top_proportion(mock_litellm):
    text = "This is a test text."
    model = "test-model"
    result = sandwich_tokens(text, model, max_tokens=1024, top_proportion=0.3)
    assert result == "decoded [...] decoded"
