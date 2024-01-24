import os
import textwrap

import openai

import llm_utils


def get_model() -> str:
    all_models = ["gpt-4", "gpt-3.5-turbo"]

    if not "OPENAI_API_MODEL" in os.environ:
        model = "gpt-4"
    else:
        model = os.environ["OPENAI_API_MODEL"]
        if model not in all_models:
            print(
                f'The environment variable OPENAI_API_MODEL is currently set to "{model}".'
            )
            print(f"The only valid values are {all_models}.")
            return ""

    return model


def explain(source_code: str, traceback: str, exception: str, really_run=True) -> None:
    user_prompt = f"""
Explain what the root cause of this error is, given the following source code
context for each stack frame and a traceback, and propose a fix. In your
response, never refer to the frames given below (as in, 'frame 0'). Instead,
always refer only to specific lines and filenames of source code.

Source code for each stack frame:
```
{source_code}
```

Traceback:
{traceback}

Stop reason: {exception}
    """.strip()

    model = get_model()
    if not model:
        return

    input_tokens = llm_utils.count_tokens(model, user_prompt)

    if not really_run:
        print(user_prompt)
        print(f"Total input tokens: {input_tokens}")
        return

    try:
        client = openai.OpenAI(timeout=30)
    except openai.OpenAIError:
        print("You need an OpenAI key to use this tool.")
        print("You can get a key here: https://platform.openai.com/api-keys")
        print("Set the environment variable OPENAI_API_KEY to your key value.")
        return

    try:
        completion = client.chat.completions.create(
            model=model, messages=[{"role": "user", "content": user_prompt}]
        )
    except openai.NotFoundError:
        print(f"'{model}' either does not exist or you do not have access to it.")
        return
    except openai.RateLimitError:
        print("You have exceeded a rate limit or have no remaining funds.")
        return
    except openai.APITimeoutError:
        print("The OpenAI API timed out.")
        return

    text = completion.choices[0].message.content
    print(llm_utils.word_wrap_except_code_blocks(text))

    input_tokens = completion.usage.prompt_tokens
    output_tokens = completion.usage.completion_tokens
    cost = llm_utils.calculate_cost(input_tokens, output_tokens, model)
    print(f"\n(Total cost: approximately ${cost:.2f} USD.)")
