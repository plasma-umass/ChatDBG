# file src/chatdbg/util/config.py:32-37
# lines [32, 33, 34, 36, 37]
# branches []

import pytest
from chatdbg.util.config import DBGParser

def test_DBGParser_error():
    parser = DBGParser()
    with pytest.raises(Exception) as exc_info:
        parser.error("test_error_message")
    assert "Error: test_error_message\n" in str(exc_info.value)
