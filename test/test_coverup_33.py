# file src/chatdbg/util/text.py:40-41
# lines [40, 41]
# branches []

import pytest
from chatdbg.util.text import fill_to_width

def test_fill_to_width_single_line():
    text = "Hello"
    width = 10
    expected = "Hello     "
    assert fill_to_width(text, width) == expected

def test_fill_to_width_multiple_lines():
    text = "Hello\nWorld"
    width = 10
    expected = "Hello     \nWorld     "
    assert fill_to_width(text, width) == expected

def test_fill_to_width_empty_string():
    text = ""
    width = 10
    expected = "          "
    assert fill_to_width(text, width) == expected

def test_fill_to_width_no_width_specified():
    text = "Hello"
    expected = "Hello" + " " * (80 - len("Hello"))
    assert fill_to_width(text) == expected

def test_fill_to_width_exact_width():
    text = "Hello"
    width = 5
    expected = "Hello"
    assert fill_to_width(text, width) == expected

def test_fill_to_width_zero_width():
    text = "Hello"
    width = 0
    expected = "Hello"
    assert fill_to_width(text, width) == expected

def test_fill_to_width_negative_width():
    text = "Hello"
    width = -5
    expected = "Hello"
    assert fill_to_width(text, width) == expected
