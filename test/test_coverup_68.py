# file src/chatdbg/pdb_util/locals.py:66-71
# lines [66, 67, 68, 69, 70, 71]
# branches []

import pytest
from chatdbg.pdb_util.locals import _is_iterable

def test_is_iterable_with_iterable_object():
    assert _is_iterable([1, 2, 3]) == True

def test_is_iterable_with_non_iterable_object():
    assert _is_iterable(1) == False

# Clean up is not necessary because the function does not modify any state
