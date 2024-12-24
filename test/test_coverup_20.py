# file src/chatdbg/pdb/text.py:5-11
# lines [5, 7, 8, 9, 10, 11]
# branches ['7->8', '7->9', '9->10', '9->11']

import pytest

from chatdbg.custom_pdb.text import make_arrow

def test_make_arrow_with_pad_greater_than_two():
    # Test with pad greater than 2
    arrow = make_arrow(5)
    assert arrow == '---> ', "Arrow with pad greater than 2 should have dashes and a '> '"

def test_make_arrow_with_pad_equal_two():
    # Test with pad equal to 2
    arrow = make_arrow(2)
    assert arrow == '> ', "Arrow with pad equal to 2 should be '> '"

def test_make_arrow_with_pad_equal_one():
    # Test with pad equal to 1
    arrow = make_arrow(1)
    assert arrow == '>', "Arrow with pad equal to 1 should be '>'"

def test_make_arrow_with_pad_less_than_one():
    # Test with pad less than 1
    arrow = make_arrow(0)
    assert arrow == '', "Arrow with pad less than 1 should be an empty string"
