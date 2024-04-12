# file src/chatdbg/assistant/assistant.py:181-188
# lines [181, 186, 187, 188]
# branches []

import json
import pytest
from chatdbg.assistant.assistant import Assistant

# Assuming the Assistant class has an __init__ method that requires 'instructions' argument
# and initializes self._functions. If not, the Assistant class definition should be updated accordingly.

class MockAssistant(Assistant):
    def __init__(self):
        self._functions = {}

def test_add_function_with_valid_json_schema():
    assistant = MockAssistant()

    def sample_function():
        """
        {
            "name": "sample_function",
            "description": "A sample function for testing"
        }
        """
    
    assistant._add_function(sample_function)
    assert "sample_function" in assistant._functions
    assert assistant._functions["sample_function"]["function"] == sample_function
    assert assistant._functions["sample_function"]["schema"]["description"] == "A sample function for testing"

def test_add_function_with_invalid_json_schema():
    assistant = MockAssistant()

    def sample_function():
        """
        {
            "description": "A sample function with invalid schema"
        }
        """
    
    with pytest.raises(AssertionError) as excinfo:
        assistant._add_function(sample_function)
    assert "Bad JSON in docstring for function tool." in str(excinfo.value)
