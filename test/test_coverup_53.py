# file src/chatdbg/util/trim.py:33-34
# lines [34]
# branches []

import pytest
from chatdbg.util.trim import _sum_kept_chunks

# Mocking the _sum_messages function
def mock_sum_messages(messages, model):
    return len(messages)

# Test function to cover the missing line 34
def test_sum_kept_chunks(monkeypatch):
    # Patch the _sum_messages function with the mock
    monkeypatch.setattr("chatdbg.util.trim._sum_messages", mock_sum_messages)

    # Define test data
    chunks = [
        (['msg1', 'msg2'], True),
        (['msg3'], False),
        (['msg4', 'msg5', 'msg6'], True)
    ]
    model = None  # Placeholder for the model, not used in this test

    # Call the function under test
    result = _sum_kept_chunks(chunks, model)

    # Assert the expected result
    assert result == 5, "The sum of kept chunks should be 5"

# Run the test
def test_sum_kept_chunks_no_kept(monkeypatch):
    # Patch the _sum_messages function with the mock
    monkeypatch.setattr("chatdbg.util.trim._sum_messages", mock_sum_messages)

    # Define test data where no chunks are kept
    chunks = [
        (['msg1', 'msg2'], False),
        (['msg3'], False)
    ]
    model = None  # Placeholder for the model, not used in this test

    # Call the function under test
    result = _sum_kept_chunks(chunks, model)

    # Assert the expected result
    assert result == 0, "The sum of kept chunks should be 0"
