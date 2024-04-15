# file src/chatdbg/util/history.py:1-22
# lines [1, 3, 4, 5, 7, 8, 10, 11, 13, 14, 15, 16, 18, 20, 21, 22]
# branches ['15->16', '15->18']

import pytest
from chatdbg.util.history import CommandHistory

@pytest.fixture
def command_history():
    return CommandHistory(prompt=">>> ")

def test_append_and_str_with_output(command_history):
    command_history.append("print('Hello, World!')", "Hello, World!")
    assert str(command_history) == ">>> print('Hello, World!')\nHello, World!"

def test_append_and_str_without_output(command_history):
    command_history.append("pass", "")
    assert str(command_history) == ">>> pass"

def test_clear(command_history):
    command_history.append("print('Hello, World!')", "Hello, World!")
    command_history.clear()
    assert str(command_history) == ""
