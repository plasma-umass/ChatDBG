import copy
import warnings
from typing import Dict, List

with warnings.catch_warnings():
    warnings.simplefilter("ignore")
    import litellm


def sandwich_tokens(
    text: str, model: str, max_tokens: int = 1024, top_proportion: float = 0.5
) -> str:
    if max_tokens == None:
        return text
    tokens = litellm.encode(model, text)
    if len(tokens) <= max_tokens:
        return text
    else:
        total_len = max_tokens - 5  # some slop for the ...
        top_len = int(top_proportion * total_len)
        bot_start = len(tokens) - (total_len - top_len)
        return (
            litellm.decode(model, tokens[0:top_len])
            + " [...] "
            + litellm.decode(model, tokens[bot_start:])
        )


def _sum_messages(messages, model):
    return litellm.token_counter(model, messages=messages)


def _sum_kept_chunks(chunks, model):
    return sum(_sum_messages(messages, model) for (messages, kept) in chunks if kept)


def _extract(messages, model, tool_call_ids):
    tools = []
    other = []
    for m in messages:
        if m.get("tool_call_id", -1) in tool_call_ids:
            content = sandwich_tokens(m["content"], model, 512, 1.0)
            # print(len(litellm.encode(model, m['content'])), '->', len(litellm.encode(model, content)))
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
    messages: List[Dict[str, str]],  # list of JSON objects encoded as dicts
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

    max_tokens_for_model = litellm.model_cost[model]["max_input_tokens"]
    max_tokens = int(max_tokens_for_model * trim_ratio)

    if litellm.token_counter(model, messages=messages) < max_tokens:
        return messages

    chunks = _chunkify(messages=messages, model=model)
    # print("0", sum_all_chunks(chunks, model), max_tokens)

    # 1. System messages
    chunks = [(m, b or m[0]["role"] == "system") for (m, b) in chunks]
    # print("1", sum_kept_chunks(chunks, model))

    # 2. First User Message
    for i in range(len(chunks)):
        messages, kept = chunks[i]
        if messages[0]["role"] == "user":
            chunks[i] = (messages, True)
    # print("2", sum_kept_chunks(chunks, model))

    # 3. Fill it up
    for i in range(len(chunks))[::-1]:
        messages, kept = chunks[i]
        if kept:
            # print('+')
            continue
        elif (
            _sum_kept_chunks(chunks, model) + _sum_messages(messages, model)
            < max_tokens
        ):
            # print('-', len(messages))
            chunks[i] = (messages, True)
        else:
            # print("N", sum_kept_chunks(chunks, model), sum_messages(messages, model))
            break

    # print("3", sum_kept_chunks(chunks, model))

    assert (
        _sum_kept_chunks(chunks, model) < max_tokens
    ), f"New conversation too big {_sum_kept_chunks(chunks, model)} vs {max_tokens}!"

    return [m for (messages, kept) in chunks if kept for m in messages]
