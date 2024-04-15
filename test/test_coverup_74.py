# file src/chatdbg/util/prompts.py:51-56
# lines [51, 52, 53, 54, 55]
# branches []

import pytest
from chatdbg.util.prompts import build_followup_prompt

# Assuming the existence of the following helper functions:
# `_concat_prompt`, `_wrap_it`, and `_user_text_it`
# Since they are not provided, we'll mock these helpers for the test.

@pytest.fixture
def mock_helpers(mocker):
    mocker.patch('chatdbg.util.prompts._concat_prompt', return_value='concatenated_prompt')
    mocker.patch('chatdbg.util.prompts._wrap_it', side_effect=lambda title, text: text)
    mocker.patch('chatdbg.util.prompts._user_text_it', side_effect=lambda text: text)

def test_build_followup_prompt(mock_helpers):
    history = "History content"
    extra = "Extra content"
    user_text = "User input text"

    result = build_followup_prompt(history, extra, user_text)

    from chatdbg.util.prompts import _concat_prompt, _wrap_it, _user_text_it

    # Assertions to ensure the correct calls were made and result is as expected
    _wrap_it.assert_any_call("This is the history of some debugger commands I ran", history)
    _wrap_it.assert_any_call("", extra)
    _user_text_it.assert_called_once_with(user_text)
    _concat_prompt.assert_called_once_with(history, extra, user_text)
    assert result == 'concatenated_prompt', "The result should be the return value from _concat_prompt"
