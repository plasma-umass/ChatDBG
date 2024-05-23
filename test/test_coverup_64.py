# file src/chatdbg/util/trim.py:64-125
# lines [81, 83, 84, 86, 87, 89, 93, 97, 98, 99, 100, 104, 105, 106, 108, 110, 111, 114, 117, 121, 122, 123, 125]
# branches ['86->87', '86->89', '97->98', '97->104', '99->97', '99->100', '104->105', '104->121', '106->108', '106->109', '109->114', '109->117']

import pytest
from chatdbg.util.trim import trim_messages

# Mock the dependencies within the trim_messages function
@pytest.fixture
def mock_litellm(mocker):
    mock_model_cost = {
        "small": {"max_input_tokens": 100},
        "medium": {"max_input_tokens": 200},
        "large": {"max_input_tokens": 300}
    }
    mocker.patch('chatdbg.util.trim.litellm.model_cost', mock_model_cost)
    mocker.patch('chatdbg.util.trim.litellm.token_counter', return_value=150)
    mocker.patch('chatdbg.util.trim._chunkify', return_value=[
        ([{"role": "system", "text": "System message"}], True),
        ([{"role": "user", "text": "User message"}], False),
        ([{"role": "tool", "text": "Tool message"}], False),
    ])
    mocker.patch('chatdbg.util.trim._sum_kept_chunks', return_value=50)
    mocker.patch('chatdbg.util.trim._sum_messages', side_effect=lambda msgs, _: len(msgs))
    return mock_model_cost

# Test function to cover lines 81-125
def test_trim_messages_full_coverage(mock_litellm):
    messages = [
        {"role": "system", "text": "System message"},
        {"role": "user", "text": "User message"},
        {"role": "tool", "text": "Tool message"}
    ]
    model = "medium"
    trim_ratio = 0.75

    # Execute the function with the given parameters
    trimmed = trim_messages(messages, model, trim_ratio)

    # Perform assertions to check postconditions
    assert isinstance(trimmed, list)
    # Check that the result is a subset of the original messages
    assert all(msg in messages for msg in trimmed)
    # Check that system messages are kept
    assert any(msg['role'] == 'system' for msg in trimmed)
    # Check that the first user message is kept
    assert any(msg['role'] == 'user' for msg in trimmed)
    # Check that we do not exceed max_tokens
    assert len(trimmed) <= int(mock_litellm[model]["max_input_tokens"] * trim_ratio)
