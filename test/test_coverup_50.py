# file src/chatdbg/util/text.py:20-28
# lines [24, 25, 26, 27, 28]
# branches ['24->25', '24->28']

import pytest
from chatdbg.util.text import truncate_proportionally

def test_truncate_proportionally_full_coverage():
    # Test case where text length is less than maxlen
    short_text = "Hello, World!"
    assert truncate_proportionally(short_text) == short_text

    # Test case where text length is more than maxlen
    long_text = "a" * 32005
    truncated = truncate_proportionally(long_text)
    pre_length = int((32000 - 5) * 0.5)
    post_length = 32000 - 5 - pre_length
    assert truncated.startswith("a" * pre_length)
    assert truncated.endswith("a" * post_length)
    assert "[...]" in truncated
    assert len(truncated) == 32000

    # Test case where top_proportion is 0
    truncated = truncate_proportionally(long_text, top_proportion=0)
    assert truncated.startswith("[...]")
    assert truncated.endswith("a" * (32000 - 5))
    assert len(truncated) == 32000

    # Test case where top_proportion is 1
    truncated = truncate_proportionally(long_text, top_proportion=1)
    assert truncated.startswith("a" * (32000 - 5))
    assert truncated.endswith("[...]")
    assert len(truncated) == 32000

    # Test case where maxlen is less than the length of the placeholder
    very_short_maxlen = 4
    truncated = truncate_proportionally(long_text, maxlen=very_short_maxlen)
    assert truncated == "[...]"
    assert len(truncated) == 5
