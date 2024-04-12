# file src/chatdbg/assistant/listeners.py:5-49
# lines [5, 6, 12, 13, 15, 16, 20, 21, 23, 24, 26, 27, 29, 30, 34, 35, 37, 38, 40, 41, 45, 46, 48, 49]
# branches []

import pytest
from chatdbg.assistant.listeners import BaseAssistantListener

class TestAssistantListener(BaseAssistantListener):
    def on_begin_dialog(self, instructions):
        self.begin_dialog_called = True
        self.instructions = instructions

    def on_end_dialog(self):
        self.end_dialog_called = True

    def on_begin_query(self, prompt, user_text):
        self.begin_query_called = True
        self.prompt = prompt
        self.user_text = user_text

    def on_response(self, text):
        self.response_called = True
        self.response_text = text

    def on_function_call(self, call, result):
        self.function_call_called = True
        self.function_call = call
        self.function_result = result

    def on_end_query(self, stats):
        self.end_query_called = True
        self.query_stats = stats

    def on_begin_stream(self):
        self.begin_stream_called = True

    def on_stream_delta(self, text):
        self.stream_delta_called = True
        self.stream_delta_text = text

    def on_end_stream(self):
        self.end_stream_called = True

    def on_warn(self, text):
        self.warn_called = True
        self.warn_text = text

    def on_error(self, text):
        self.error_called = True
        self.error_text = text

@pytest.fixture
def listener():
    return TestAssistantListener()

def test_on_begin_dialog(listener):
    instructions = "Test instructions"
    listener.on_begin_dialog(instructions)
    assert listener.begin_dialog_called
    assert listener.instructions == instructions

def test_on_end_dialog(listener):
    listener.on_end_dialog()
    assert listener.end_dialog_called

def test_on_begin_query(listener):
    prompt = "Test prompt"
    user_text = "User input"
    listener.on_begin_query(prompt, user_text)
    assert listener.begin_query_called
    assert listener.prompt == prompt
    assert listener.user_text == user_text

def test_on_response(listener):
    text = "Test response"
    listener.on_response(text)
    assert listener.response_called
    assert listener.response_text == text

def test_on_function_call(listener):
    call = "function_name"
    result = "function_result"
    listener.on_function_call(call, result)
    assert listener.function_call_called
    assert listener.function_call == call
    assert listener.function_result == result

def test_on_end_query(listener):
    stats = {"time_taken": 10}
    listener.on_end_query(stats)
    assert listener.end_query_called
    assert listener.query_stats == stats

def test_on_begin_stream(listener):
    listener.on_begin_stream()
    assert listener.begin_stream_called

def test_on_stream_delta(listener):
    text = "Stream delta text"
    listener.on_stream_delta(text)
    assert listener.stream_delta_called
    assert listener.stream_delta_text == text

def test_on_end_stream(listener):
    listener.on_end_stream()
    assert listener.end_stream_called

def test_on_warn(listener):
    text = "Warning message"
    listener.on_warn(text)
    assert listener.warn_called
    assert listener.warn_text == text

def test_on_error(listener):
    text = "Error message"
    listener.on_error(text)
    assert listener.error_called
    assert listener.error_text == text
