# file src/chatdbg/assistant/assistant.py:25-28
# lines [25, 26, 27, 28]
# branches []

import pytest
import string
from chatdbg.assistant.assistant import remove_non_printable_chars

def test_remove_non_printable_chars_with_printable():
    printable_text = "Hello, World!"
    result = remove_non_printable_chars(printable_text)
    assert result == printable_text

def test_remove_non_printable_chars_with_non_printable():
    non_printable_text = "Hello\x00World!"
    expected_result = "HelloWorld!"
    result = remove_non_printable_chars(non_printable_text)
    assert result == expected_result

def test_remove_non_printable_chars_with_mixed_characters():
    mixed_text = "Hello\x00, \x01World\x02!"
    expected_result = "Hello, World!"
    result = remove_non_printable_chars(mixed_text)
    assert result == expected_result

def test_remove_non_printable_chars_empty_string():
    empty_string = ""
    result = remove_non_printable_chars(empty_string)
    assert result == empty_string

def test_remove_non_printable_chars_all_non_printable():
    all_non_printable = "".join(chr(i) for i in range(256) if chr(i) not in string.printable)
    result = remove_non_printable_chars(all_non_printable)
    assert result == ""
