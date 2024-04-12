# file src/chatdbg/util/trim.py:68-128
# lines [68, 69, 70, 71, 72, 85, 87, 88, 90, 91, 93, 97, 101, 102, 103, 104, 108, 109, 110, 112, 114, 117, 120, 124, 125, 126, 128]
# branches ['90->91', '90->93', '101->102', '101->108', '103->101', '103->104', '108->109', '108->124', '110->112', '110->113', '113->117', '113->120']

import pytest
from unittest.mock import MagicMock, patch
from chatdbg.util.trim import trim_messages

# Mocking the external dependencies and global variables
litellm = MagicMock()
litellm.model_cost = {
    "test_model": {"max_tokens": 100},
    "another_model": {"max_tokens": 200},
}
litellm.token_counter = lambda model, messages: sum(len(m['content']) for m in messages)
chunkify = lambda messages, model: [([m], False) for m in messages]
sum_messages = lambda messages, model: sum(len(m['content']) for m in messages)
sum_kept_chunks = lambda chunks, model: sum(sum_messages(m, model) for m, kept in chunks if kept)
sum_all_chunks = lambda chunks, model: sum(sum_messages(m, model) for m, kept in chunks)

# Test to cover the branch where token count is less than max_tokens
def test_trim_messages_no_trimming_required():
    messages = [
        {"role": "user", "content": "Hello"},
        {"role": "system", "content": "System message"},
        {"role": "user", "content": "How are you?"},
    ]
    model = "test_model"
    with patch('chatdbg.util.trim.litellm', litellm), \
         patch('chatdbg.util.trim.chunkify', chunkify), \
         patch('chatdbg.util.trim.sum_messages', sum_messages), \
         patch('chatdbg.util.trim.sum_kept_chunks', sum_kept_chunks), \
         patch('chatdbg.util.trim.sum_all_chunks', sum_all_chunks):
        trimmed_messages = trim_messages(messages, model)
    assert trimmed_messages == messages, "No messages should be trimmed if under max_tokens"
