# file src/chatdbg/pdb/text.py:14-16
# lines [14, 15, 16]
# branches []

import re
import pytest

# Assuming the file structure is as follows:
# chatdbg/
# ├── __init__.py
# └── pdb/
#     ├── __init__.py
#     └── text.py

# The content of chatdbg/pdb/text.py is:
# def strip_color(s: str) -> str:
#     ansi_escape = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")
#     return ansi_escape.sub("", s)

# Here is the test script for the above function:

from chatdbg.custom_pdb.text import strip_color

def test_strip_color_with_ansi_escape_sequences():
    # ANSI escape sequences for colors and formatting
    colored_string = "\x1b[31mThis is red\x1b[0m and this is \x1b[1;32mgreen\x1b[0m"
    expected_result = "This is red and this is green"
    
    # Call the function with a string containing ANSI escape sequences
    result = strip_color(colored_string)
    
    # Assert that the result is the string without ANSI escape sequences
    assert result == expected_result

def test_strip_color_without_ansi_escape_sequences():
    # String without ANSI escape sequences
    plain_string = "This string has no color codes"
    expected_result = plain_string
    
    # Call the function with a plain string
    result = strip_color(plain_string)
    
    # Assert that the result is the same as the input string
    assert result == expected_result

# The following tests are to ensure full coverage of the regex pattern

def test_strip_color_with_ansi_cursor_movement():
    # ANSI escape sequences for cursor movement
    cursor_movement_string = "\x1b[3D\x1b[2C"
    expected_result = ""
    
    # Call the function with a string containing ANSI cursor movement sequences
    result = strip_color(cursor_movement_string)
    
    # Assert that the result is an empty string
    assert result == expected_result

def test_strip_color_with_ansi_clear_screen():
    # ANSI escape sequences for clearing the screen
    clear_screen_string = "\x1b[2J\x1b[1;1H"
    expected_result = ""
    
    # Call the function with a string containing ANSI clear screen sequences
    result = strip_color(clear_screen_string)
    
    # Assert that the result is an empty string
    assert result == expected_result

def test_strip_color_with_ansi_complex_sequences():
    # ANSI escape sequences with complex patterns
    complex_sequence_string = "\x1b[38;5;82m\x1b[48;5;24m"
    expected_result = ""
    
    # Call the function with a string containing complex ANSI sequences
    result = strip_color(complex_sequence_string)
    
    # Assert that the result is an empty string
    assert result == expected_result

# No top-level code is included as per the instructions.
