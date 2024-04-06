
import copy
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
        bot_len = int((1 - top_proportion) * total_len)
        return (
            litellm.decode(model, tokens[0:top_len])
            + " [...] "
            + litellm.decode(model, tokens[-bot_len:])
        )

def sum_messages(messages, model):
    return litellm.token_counter(model, messages=messages)

def sum_kept_chunks(chunks, model):
    return sum(sum_messages(messages, model) for (messages, kept) in chunks if kept)

def sum_all_chunks(chunks, model):
    return sum(sum_messages(messages, model) for (messages, kept) in chunks)

def extract(messages, model, tool_call_ids):
    tools = [ ]
    other = [ ]
    for m in messages:
        if m.get('tool_call_id', -1) in tool_call_ids:
            m['content'] = sandwich_tokens(m['content'], model, 512, 1.0)
            tools += [ m ]
        else:
            other += [ m ] 
    return tools, other

def chunkify(messages, model):
    if not messages:
        return [ ]
    message = messages[0]
    if 'tool_calls' not in message:
        return [ ([message], False) ] + chunkify(messages[1:], model)
    else:
        ids = [ tool_call['id'] for tool_call in message['tool_calls']]
        tools, other = extract(messages[1:], model, ids)
        return [ ([message] + tools, False) ] + chunkify(other, model)


def trim_messages(
    messages,
    model,
    trim_ratio: float = 0.75,
):
    """
    Strategy:
    - chunk messages:
        - single message, or
        - tool request and all the tool responses
    - keep the system messages
    - keep the first user message
    - go most recent to oldest, keeping chunks until we are at the limit
    
    Also, shorten tool call results along the way.
    - """

    messages = copy.deepcopy(messages)

    max_tokens_for_model = litellm.model_cost[model]["max_tokens"]
    max_tokens = int(max_tokens_for_model * trim_ratio)

    if litellm.token_counter(model, messages=messages) < max_tokens:
        return messages

    chunks = chunkify(messages=messages, model=model)
    # print("0", sum_all_chunks(chunks, model))

    # 1. System messages
    chunks = [ (m, b or m[0]['role'] == 'system') for (m,b) in chunks]
    # print("1", sum_kept_chunks(chunks, model))

    # 2. First User Message
    for i in range(len(chunks)):
        messages, kept = chunks[i]
        if messages[0]['role'] == 'user':
            chunks[i] = (messages, True)
    # print("2", sum_kept_chunks(chunks, model))
    
    # 3. Fill it up
    for i in range(len(chunks))[::-1]:
        messages, kept = chunks[i]
        if kept:
            continue
        elif sum_kept_chunks(chunks, model) + sum_messages(messages, model) < max_tokens:
            chunks[i] = (messages, True)
        else:
            break

    # print("3", sum_kept_chunks(chunks, model))

    assert sum_kept_chunks(chunks, model) < max_tokens, f"New conversation too big {sum_kept_chunks(chunks, model)} vs {max_tokens}!"

    return [ m for (messages, kept) in chunks for m in messages if kept]
