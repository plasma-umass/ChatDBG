# file src/chatdbg/util/prompts.py:26-27
# lines [26, 27]
# branches []

import pytest
from chatdbg.util.prompts import _user_text_it

def test_user_text_it_non_empty():
    # Test with non-empty string
    user_text = "Non-empty string"
    result = _user_text_it(user_text)
    assert result == user_text

def test_user_text_it_empty():
    # Test with empty string
    user_text = ""
    result = _user_text_it(user_text)
    assert result == "What's the bug? Give me a fix."
