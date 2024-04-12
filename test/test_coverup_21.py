# file src/chatdbg/assistant/assistant.py:128-132
# lines [128, 129, 130, 132]
# branches ['129->130', '129->132']

import pytest
from chatdbg.assistant.assistant import Assistant
from unittest.mock import MagicMock

# Assuming Assistant class has an __init__ that requires 'instructions' argument
# We will create a MagicMock object for 'instructions' to pass to the Assistant constructor

# Test to cover the branch where stats["completed"] is True
def test_report_completed(capsys):
    instructions = MagicMock()
    assistant = Assistant(instructions)
    assistant._report({"completed": True})
    captured = capsys.readouterr()
    assert captured.out == "\n"

# Test to cover the branch where stats["completed"] is False
def test_report_interrupted(capsys):
    instructions = MagicMock()
    assistant = Assistant(instructions)
    assistant._report({"completed": False})
    captured = capsys.readouterr()
    assert captured.out == "[Chat Interrupted]\n"
