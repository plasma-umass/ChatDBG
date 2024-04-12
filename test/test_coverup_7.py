# file src/chatdbg/assistant/assistant.py:84-126
# lines [84, 100, 101, 103, 104, 105, 106, 108, 109, 111, 112, 113, 114, 115, 116, 117, 118, 120, 121, 122, 123, 125, 126]
# branches ['105->106', '105->108']

import pytest
import time
import openai
from unittest.mock import MagicMock

# Assuming the Assistant class is in a module named chatdbg.assistant.assistant
# and it requires 'instructions' argument in the constructor
from chatdbg.assistant.assistant import Assistant

@pytest.fixture
def assistant():
    # Create a mock Assistant instance with a dummy 'instructions' argument
    assistant = Assistant(instructions="dummy instructions")
    assistant._model = "test-model"
    assistant._stream = False
    assistant._broadcast = MagicMock()
    assistant._warn_about_exception = MagicMock()
    return assistant

def test_query_batch_query_success(assistant, monkeypatch):
    # Mock the _batch_query method to return a successful response
    monkeypatch.setattr(assistant, '_batch_query', lambda prompt, user_text: {"cost": 1.0})
    # Execute the query method
    result = assistant.query("test prompt", "test user text")
    # Assertions to check if the query was successful and the stats are correct
    assert result["completed"] == True
    assert result["cost"] == 1.0
    assert "time" in result
    assert result["model"] == "test-model"
    assert "message" in result
    # Check if the broadcast method was called correctly
    assistant._broadcast.assert_called_with("on_end_query", result)

def test_query_streamed_query_success(assistant, monkeypatch):
    # Set the _stream attribute to True to test the streamed query path
    assistant._stream = True
    # Mock the _streamed_query method to return a successful response
    monkeypatch.setattr(assistant, '_streamed_query', lambda prompt, user_text: {"cost": 2.0})
    # Execute the query method
    result = assistant.query("test prompt", "test user text")
    # Assertions to check if the query was successful and the stats are correct
    assert result["completed"] == True
    assert result["cost"] == 2.0
    assert "time" in result
    assert result["model"] == "test-model"
    assert "message" in result
    # Check if the broadcast method was called correctly
    assistant._broadcast.assert_called_with("on_end_query", result)

def test_query_openai_error(assistant, monkeypatch):
    # Mock the _batch_query method to raise an OpenAIError
    def raise_openai_error(prompt, user_text):
        raise openai.OpenAIError("OpenAI error")
    monkeypatch.setattr(assistant, '_batch_query', raise_openai_error)
    # Execute the query method
    result = assistant.query("test prompt", "test user text")
    # Assertions to check if the query was unsuccessful and the stats are correct
    assert result["completed"] == False
    assert "message" in result
    assert "[Exception: OpenAI error]" in result["message"]
    # Check if the broadcast method was called correctly
    assistant._broadcast.assert_called_with("on_end_query", result)

def test_query_keyboard_interrupt(assistant, monkeypatch):
    # Mock the _batch_query method to raise a KeyboardInterrupt
    def raise_keyboard_interrupt(prompt, user_text):
        raise KeyboardInterrupt()
    monkeypatch.setattr(assistant, '_batch_query', raise_keyboard_interrupt)
    # Execute the query method
    result = assistant.query("test prompt", "test user text")
    # Assertions to check if the query was interrupted and the stats are correct
    assert result["completed"] == False
    assert "message" in result
    assert "[Chat Interrupted]" in result["message"]
    # Check if the broadcast method was called correctly
    assistant._broadcast.assert_called_with("on_end_query", result)

def test_query_unexpected_exception(assistant, monkeypatch):
    # Mock the _batch_query method to raise an unexpected exception
    def raise_unexpected_exception(prompt, user_text):
        raise Exception("Unexpected error")
    monkeypatch.setattr(assistant, '_batch_query', raise_unexpected_exception)
    # Execute the query method
    result = assistant.query("test prompt", "test user text")
    # Assertions to check if the query encountered an unexpected exception and the stats are correct
    assert result["completed"] == False
    assert "message" in result
    assert "[Exception: Unexpected error]" in result["message"]
    # Check if the broadcast method was called correctly
    assistant._broadcast.assert_called_with("on_end_query", result)
