import os
import tiktoken
import openai

from llm_utils import llm_utils


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


def read_lines_width() -> int:
    return 10


def read_lines(file_path: str, start_line: int, end_line: int) -> str:
    """
    Read lines from a file and return a string containing the lines between start_line and end_line.

    Args:
        file_path (str): The path of the file to read.
        start_line (int): The line number of the first line to include (1-indexed).
        end_line (int): The line number of the last line to include.

    Returns:
        str: A string containing the lines between start_line and end_line.

    """
    # open the file for reading
    with open(file_path, "r") as f:
        # read all the lines from the file
        lines = f.readlines()
        # remove trailing newline characters
        lines = [line.rstrip() for line in lines]
        # add line numbers
        lines = [f"   {index+1:<6} {line}" for index, line in enumerate(lines)]
    # convert start_line to 0-based indexing
    start_line = max(0, start_line - 1)
    # ensure end_line is within range
    end_line = min(len(lines), end_line)
    # return the requested lines as a string
    return "\n".join(lines[start_line:end_line])


def num_tokens_from_string(string: str, model: str) -> int:
    """Returns the number of tokens in a text string."""
    encoding = tiktoken.encoding_for_model(model)
    num_tokens = len(encoding.encode(string))
    return num_tokens


def explain(source_code: str, traceback: str, exception: str, really_run=True) -> None:
    import httpx

    user_prompt = "Explain what the root cause of this error is, given the following source code context for each stack frame and a traceback, and propose a fix. In your response, never refer to the frames given below (as in, 'frame 0'). Instead, always refer only to specific lines and filenames of source code.\n"
    user_prompt += "\n"
    user_prompt += "Source code for each stack frame:\n```\n"
    user_prompt += source_code + "\n```\n"
    user_prompt += traceback + "\n\n"
    user_prompt += "stop reason = " + exception + "\n"
    text = ""

    model = get_model()
    if not model:
        return

    input_tokens = num_tokens_from_string(user_prompt, model)
    
    if not really_run:
        print(user_prompt)
        print(f"Total input tokens: {input_tokens}")
        return

    try:
        completion = openai.ChatCompletion.create(
            model=model,
            request_timeout=30,
            messages=[{"role": "user", "content": user_prompt}],
        )
        text = completion.choices[0].message.content
        input_tokens = completion.usage.prompt_tokens
        output_tokens = completion.usage.completion_tokens
        context_window = "8K" if model == "gpt-4" else "4K" # FIXME: true as of Oct 3, 2023
        cost = llm_utils.calculate_cost(input_tokens, output_tokens, model, context_window)
        text += f"\n(Total cost: approximately ${cost:.2f} USD.)"
        print(llm_utils.word_wrap_except_code_blocks(text))
    except openai.error.AuthenticationError:
        print(
            "You need a valid OpenAI key to use ChatDBG. You can get a key here: https://openai.com/api/"
        )
        print("Set the environment variable OPENAI_API_KEY to your key value.")
