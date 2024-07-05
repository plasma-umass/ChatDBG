# file src/chatdbg/pdb/text.py:19-27
# lines [23, 24, 25, 26, 27]
# branches ['23->24', '23->27']

import pytest
from chatdbg.custom_pdb.text import truncate_proportionally

def test_truncate_proportionally_full_coverage():
    # Test case where text is longer than maxlen and top_proportion is default
    text = "a" * 32001
    truncated = truncate_proportionally(text)
    assert truncated == "a" * 15998 + "..." + "a" * 15999
    assert len(truncated) == 32000

    # Test case where text is longer than maxlen and top_proportion is 0
    text = "a" * 32001
    truncated = truncate_proportionally(text, top_proportion=0)
    assert truncated == "..." + "a" * 31997
    assert len(truncated) == 32000

    # Test case where text is longer than maxlen and top_proportion is 1
    text = "a" * 32001
    truncated = truncate_proportionally(text, top_proportion=1)
    assert truncated == "a" * 31997 + "..."
    assert len(truncated) == 32000

    # Test case where text is not longer than maxlen
    text = "a" * 31999
    truncated = truncate_proportionally(text)
    assert truncated == text
    assert len(truncated) == len(text)

    # Test case where text is exactly maxlen
    text = "a" * 32000
    truncated = truncate_proportionally(text)
    assert truncated == text
    assert len(truncated) == len(text)

# No top-level code is included, as requested.
