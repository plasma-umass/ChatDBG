# file src/chatdbg/util/trim.py:33-34
# lines [33, 34]
# branches []

import pytest
from chatdbg.util.trim import sum_kept_chunks

# Mock function to replace sum_messages
def mock_sum_messages(messages, model):
    return len(messages)

# Test function to cover the case where 'kept' is True
def test_sum_kept_chunks_with_kept_true(monkeypatch):
    # Replace the original sum_messages with the mock function
    monkeypatch.setattr("chatdbg.util.trim.sum_messages", mock_sum_messages)
    
    # Define chunks with 'kept' as True
    chunks = [(['message1', 'message2'], True), (['message3'], True)]
    
    # Call the function under test
    result = sum_kept_chunks(chunks, None)
    
    # Assert that the result is as expected
    assert result == 3  # Because there are 3 messages in total and all are kept

# Test function to cover the case where 'kept' is False
def test_sum_kept_chunks_with_kept_false(monkeypatch):
    # Replace the original sum_messages with the mock function
    monkeypatch.setattr("chatdbg.util.trim.sum_messages", mock_sum_messages)
    
    # Define chunks with 'kept' as False
    chunks = [(['message1', 'message2'], False), (['message3'], False)]
    
    # Call the function under test
    result = sum_kept_chunks(chunks, None)
    
    # Assert that the result is as expected
    assert result == 0  # Because no messages are kept

# Test function to cover the case with a mix of 'kept' True and False
def test_sum_kept_chunks_with_mixed_kept(monkeypatch):
    # Replace the original sum_messages with the mock function
    monkeypatch.setattr("chatdbg.util.trim.sum_messages", mock_sum_messages)
    
    # Define chunks with a mix of 'kept' True and False
    chunks = [(['message1', 'message2'], True), (['message3'], False), (['message4', 'message5'], True)]
    
    # Call the function under test
    result = sum_kept_chunks(chunks, None)
    
    # Assert that the result is as expected
    assert result == 4  # Because there are 4 messages kept (2 in the first chunk and 2 in the third chunk)
