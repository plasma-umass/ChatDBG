import copy

import tiktoken

from ..util import litellm


def sandwich_tokens(
    text: str, model: str, max_tokens: int = 1024, top_proportion: float = 0.5
) -> str:
    if not max_tokens:
        return text

    try:
        encoding = tiktoken.encoding_for_model(model)
    except KeyError:
        # For non-OpenAI models, use the GPT-4o encoding by default.
        encoding = tiktoken.get_encoding("o200k_base")

    tokens = encoding.encode(text)
    if len(tokens) <= max_tokens:
        return text
    else:
        total_len = max_tokens - 5  # some slop for the ...
        top_len = int(top_proportion * total_len)
        bot_start = len(tokens) - (total_len - top_len)
        return (
            encoding.decode(model, tokens[0:top_len])
            + " [...] "
            + encoding.decode(model, tokens[bot_start:])
        )


def sum_messages(messages, model):
    try:
        encoding = tiktoken.encoding_for_model(model)
    except KeyError:
        # For non-OpenAI models, use the GPT-4o encoding by default.
        encoding = tiktoken.get_encoding("o200k_base")

    # This is a lower-bound approximation, it won't match the reported usage.
    count = 0
    for message in messages:
        if "content" in message:
            count += len(encoding.encode(message["content"]))
        if "tool_calls" in message:
            for tool_call in message["tool_calls"]:
                count += len(encoding.encode(tool_call["function"]["name"]))
                count += len(encoding.encode(tool_call["function"]["arguments"]))

    return count


def _sum_kept_chunks(chunks, model):
    return sum(sum_messages(messages, model) for (messages, kept) in chunks if kept)


def _extract(messages, model, tool_call_ids):
    tools = []
    other = []
    for m in messages:
        if m.get("tool_call_id", -1) in tool_call_ids:
            content = sandwich_tokens(m["content"], model, 512, 1.0)
            m["content"] = content
            tools += [m]
        else:
            other += [m]
    return tools, other


def _chunkify(messages, model):
    if not messages:
        return []
    m = messages[0]
    if "tool_calls" not in m:
        m["content"] = sandwich_tokens(m["content"], model, 1024, 0)
        return [([m], False)] + _chunkify(messages[1:], model)
    else:
        ids = [tool_call["id"] for tool_call in m["tool_calls"]]
        tools, other = _extract(messages[1:], model, ids)
        return [([m] + tools, False)] + _chunkify(other, model)


def trim_messages(
    messages: list[dict[str, str]],  # list of JSON objects encoded as dicts
    model: str,
    trim_ratio: float = 0.75,
) -> list:
    """
    Strategy:
    - chunk messages:
        - single message, or
        - tool request and all the tool responses
    - keep the system messages
    - keep the first user message
    - go most recent to oldest, keeping chunks until we are at the limit

    Also, shorten tool call results along the way.
    -"""

    messages = copy.deepcopy(messages)

    if model in litellm.model_data:
        max_tokens_for_model = litellm.model_data[model]["max_input_tokens"]
    else:
        # Arbitrary. This is Llama 3.1/3.2/3.3 max input tokens.
        max_tokens_for_model = 128000
    max_tokens = int(max_tokens_for_model * trim_ratio)

    if sum_messages(messages, model) < max_tokens:
        return messages

    chunks = _chunkify(messages=messages, model=model)

    # 1. System messages
    chunks = [(m, b or m[0]["role"] == "system") for (m, b) in chunks]

    # 2. First User Message
    for i in range(len(chunks)):
        messages, kept = chunks[i]
        if messages[0]["role"] == "user":
            chunks[i] = (messages, True)

    # 3. Fill it up
    for i in range(len(chunks))[::-1]:
        messages, kept = chunks[i]
        if kept:
            continue
        elif (
            _sum_kept_chunks(chunks, model) + sum_messages(messages, model) < max_tokens
        ):
            chunks[i] = (messages, True)
        else:
            break

    assert (
        _sum_kept_chunks(chunks, model) < max_tokens
    ), f"New conversation too big {_sum_kept_chunks(chunks, model)} vs {max_tokens}!"

    return [m for (messages, kept) in chunks if kept for m in messages]
