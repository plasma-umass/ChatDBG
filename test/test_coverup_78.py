# file src/chatdbg/util/prompts.py:21-23
# lines [21, 22, 23]
# branches []

import pytest
from chatdbg.util.prompts import _concat_prompt

def test_concat_prompt_single_arg():
    assert _concat_prompt("line1") == "line1", "Failed to concatenate single argument"

def test_concat_prompt_multiple_args():
    assert _concat_prompt("line1", "line2", "line3") == "line1\nline2\nline3", "Failed to concatenate multiple arguments"

def test_concat_prompt_empty_string():
    assert _concat_prompt("", "line1", "") == "line1", "Failed to exclude empty strings"

def test_concat_prompt_all_empty_strings():
    assert _concat_prompt("", "", "") == "", "Failed to return empty string for all empty arguments"

def test_concat_prompt_mixed_empty_and_non_empty_strings():
    assert _concat_prompt("line1", "", "line2") == "line1\nline2", "Failed to exclude empty strings in mixed arguments"

# Running the tests
if __name__ == "__main__":
    pytest.main()
