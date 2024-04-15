# file src/chatdbg/assistant/assistant.py:307-321
# lines [309, 311, 312, 313, 314, 315, 316, 318, 319, 320]
# branches []

import pytest
from unittest.mock import MagicMock
from chatdbg.assistant.assistant import Assistant

@pytest.fixture
def assistant():
    # Setup Assistant instance with necessary mocks
    instructions = MagicMock()
    assistant = Assistant(instructions)
    assistant._trim_conversation = MagicMock()
    assistant._model = MagicMock()
    assistant._conversation = MagicMock()
    assistant._functions = MagicMock()
    assistant._timeout = MagicMock()
    assistant._logger = MagicMock()
    return assistant

def test_completion_without_stream(assistant, monkeypatch):
    # Mock the litellm.completion function
    mock_completion = MagicMock(return_value="mocked_completion")
    monkeypatch.setattr("chatdbg.assistant.assistant.litellm.completion", mock_completion)
    result = assistant._completion(stream=False)
    # Assertions to check if the function is called with the correct parameters
    mock_completion.assert_called_once_with(
        model=assistant._model,
        messages=assistant._conversation,
        tools=[{"type": "function", "function": f["schema"]} for f in assistant._functions.values()],
        timeout=assistant._timeout,
        logger_fn=assistant._logger,
        stream=False,
    )
    assert result == "mocked_completion"
    # Ensure that _trim_conversation was called
    assistant._trim_conversation.assert_called_once()

def test_completion_with_stream(assistant, monkeypatch):
    # Mock the litellm.completion function
    mock_stream_completion = MagicMock(return_value="mocked_stream_completion")
    monkeypatch.setattr("chatdbg.assistant.assistant.litellm.completion", mock_stream_completion)
    result = assistant._completion(stream=True)
    # Assertions to check if the function is called with the correct parameters
    mock_stream_completion.assert_called_once_with(
        model=assistant._model,
        messages=assistant._conversation,
        tools=[{"type": "function", "function": f["schema"]} for f in assistant._functions.values()],
        timeout=assistant._timeout,
        logger_fn=assistant._logger,
        stream=True,
    )
    assert result == "mocked_stream_completion"
    # Ensure that _trim_conversation was called
    assistant._trim_conversation.assert_called_once()
