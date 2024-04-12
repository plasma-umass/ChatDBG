# file src/chatdbg/util/printer.py:74-79
# lines [75, 76, 78, 79]
# branches ['75->76', '75->78']

import pytest
from chatdbg.util.printer import ChatDBGPrinter
from unittest.mock import MagicMock

# Assuming BaseAssistantListener is a class that ChatDBGPrinter inherits from
# and _debugger_prompt and _print are defined in ChatDBGPrinter or BaseAssistantListener

class MockDebuggerPrompt:
    def __init__(self):
        self._debugger_prompt = "Debug> "
        self._print = MagicMock()

@pytest.fixture
def printer(monkeypatch):
    monkeypatch.setattr(ChatDBGPrinter, '_print', MagicMock())
    return ChatDBGPrinter(out=MagicMock(), debugger_prompt="Debug> ", chat_prefix="", width=80)

def test_on_function_call_with_result(printer):
    call = "test_call"
    result = "test_result"
    printer.on_function_call(call, result)
    
    printer._print.assert_called_once_with("Debug> test_call\ntest_result")

def test_on_function_call_without_result(printer):
    call = "test_call"
    result = ""
    printer.on_function_call(call, result)
    
    printer._print.assert_called_once_with("Debug> test_call")
