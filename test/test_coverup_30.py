# file src/chatdbg/util/text.py:14-16
# lines [14, 15, 16]
# branches []

import re
import pytest

from chatdbg.util.text import strip_ansi

def test_strip_ansi_with_ansi_escape_sequences():
    # Test with strings containing ANSI escape sequences
    assert strip_ansi("\x1B[0;32mGreen Text\x1B[0m") == "Green Text"
    assert strip_ansi("\x1B[1;34mBlue Text\x1B[0m") == "Blue Text"
    assert strip_ansi("\x1B[0;31;42mRed on Green\x1B[0m") == "Red on Green"
    assert strip_ansi("\x1B[0mNormal Text") == "Normal Text"

def test_strip_ansi_without_ansi_escape_sequences():
    # Test with strings that do not contain ANSI escape sequences
    assert strip_ansi("No Color Text") == "No Color Text"

def test_strip_ansi_empty_string():
    # Test with an empty string
    assert strip_ansi("") == ""

def test_strip_ansi_with_non_ansi_escape_sequences():
    # Test with strings that contain non-ANSI escape sequences
    # The non-ANSI escape sequence should not be stripped
    assert strip_ansi("Other Escape Sequence") == "Other Escape Sequence"
