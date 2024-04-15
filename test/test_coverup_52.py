# file src/chatdbg/util/trim.py:29-30
# lines [30]
# branches []

import pytest
from chatdbg.util.trim import _sum_messages
from unittest.mock import MagicMock

# Assuming litellm is a module that needs to be mocked
@pytest.fixture
def mock_litellm(monkeypatch):
    mock_litellm = MagicMock()
    monkeypatch.setattr('chatdbg.util.trim.litellm', mock_litellm)
    return mock_litellm

def test_sum_messages_executes_line_30(mock_litellm):
    # Arrange
    mock_model = MagicMock()
    mock_messages = ['message1', 'message2']
    mock_litellm.token_counter.return_value = 42  # Arbitrary return value for the purpose of the test

    # Act
    result = _sum_messages(mock_messages, mock_model)

    # Assert
    mock_litellm.token_counter.assert_called_once_with(mock_model, messages=mock_messages)
    assert result == 42
