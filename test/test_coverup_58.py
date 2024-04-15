# file src/chatdbg/util/text.py:31-40
# lines [32, 33, 34, 35, 36, 39, 40]
# branches ['33->34', '33->40', '34->35', '34->39']

import pytest
from chatdbg.util.text import wrap_long_lines

def test_wrap_long_lines_short():
    input_text = "This is a short line."
    expected_output = "This is a short line."
    assert wrap_long_lines(input_text) == expected_output

def test_wrap_long_lines_exact_width():
    input_text = "This line is exactly eighty characters long, which is the default width."
    expected_output = "This line is exactly eighty characters long, which is the default width."
    assert wrap_long_lines(input_text) == expected_output

def test_wrap_long_lines_long():
    input_text = "This line is going to be wrapped because it is longer than the default width of eighty characters."
    expected_output = "This line is going to be wrapped because it is longer than the default width of\n    eighty characters."
    assert wrap_long_lines(input_text) == expected_output

def test_wrap_long_lines_with_subsequent_indent():
    input_text = "This line is going to be wrapped and will have an indent on the subsequent lines."
    expected_output = "This line is going to be wrapped and will have an indent on the subsequent\n    lines."
    assert wrap_long_lines(input_text, subsequent_indent="    ") == expected_output

def test_wrap_long_lines_multiple_lines():
    input_text = "Short line.\nThis line is going to be wrapped because it is longer than the default width of eighty characters."
    expected_output = "Short line.\nThis line is going to be wrapped because it is longer than the default width of\n    eighty characters."
    assert wrap_long_lines(input_text) == expected_output

def test_wrap_long_lines_multiple_long_lines():
    input_text = (
        "This line is going to be wrapped because it is longer than the default width of eighty characters.\n"
        "This second line is also quite long and will be wrapped accordingly to the specified width."
    )
    expected_output = (
        "This line is going to be wrapped because it is longer than the default width of\n    eighty characters.\n"
        "This second line is also quite long and will be wrapped accordingly to the\n    specified width."
    )
    assert wrap_long_lines(input_text) == expected_output
