import os
import sys
import textwrap

import openai


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


def word_wrap_except_code_blocks(text: str) -> str:
    """
    Wraps text except for code blocks.

    Splits the text into paragraphs and wraps each paragraph,
    except for paragraphs that are inside of code blocks denoted
    by ` ``` `. Returns the updated text.

    Args:
        text: The text to wrap.

    Returns:
        The wrapped text.
    """
    # Split text into paragraphs
    paragraphs = text.split("\n\n")
    wrapped_paragraphs = []
    # Check if currently in a code block.
    in_code_block = False
    # Loop through each paragraph and apply appropriate wrapping.
    for paragraph in paragraphs:
        # Check for the presence of triple quotes in the paragraph
        if "```" in paragraph:
            # Split paragraph by triple quotes
            parts = paragraph.split("```")
            for i, part in enumerate(parts):
                # If we are inside a code block, do not wrap the text
                if in_code_block:
                    wrapped_paragraphs.append(part)
                else:
                    # Otherwise, apply text wrapping to the part
                    wrapped_paragraphs.append(textwrap.fill(part))
                # Toggle the in_code_block flag for each triple quote encountered
                if i < len(parts) - 1:
                    wrapped_paragraphs.append("```")
                    in_code_block = not in_code_block
        else:
            # If the paragraph does not contain triple quotes and is not inside a code block, wrap the text
            if not in_code_block:
                wrapped_paragraphs.append(textwrap.fill(paragraph))
            else:
                wrapped_paragraphs.append(paragraph)
    # Join all paragraphs into a single string
    wrapped_text = "\n\n".join(wrapped_paragraphs)
    return wrapped_text


def word_wrap_except_code_blocks_previous(text: str) -> str:
    """Wraps text except for code blocks.

    Splits the text into paragraphs and wraps each paragraph,
    except for paragraphs that are inside of code blocks denoted
    by ` ``` `. Returns the updated text.

    Args:
        text: The text to wrap.

    Returns:
        The wrapped text.
    """
    # Split text into paragraphs
    paragraphs = text.split("\n\n")
    wrapped_paragraphs = []
    # Check if currently in a code block.
    in_code_block = False
    # Loop through each paragraph and apply appropriate wrapping.
    for paragraph in paragraphs:
        # If this paragraph starts and ends with a code block, add it as is.
        if paragraph.startswith("```") and paragraph.endswith("```"):
            wrapped_paragraphs.append(paragraph)
            continue
        # If this is the beginning of a code block add it as is.
        if paragraph.startswith("```"):
            in_code_block = True
            wrapped_paragraphs.append(paragraph)
            continue
        # If this is the end of a code block stop skipping text.
        if paragraph.endswith("```"):
            in_code_block = False
            wrapped_paragraphs.append(paragraph)
            continue
        # If we are currently in a code block add the paragraph as is.
        if in_code_block:
            wrapped_paragraphs.append(paragraph)
        else:
            # Otherwise, apply text wrapping to the paragraph.
            wrapped_paragraph = textwrap.fill(paragraph)
            wrapped_paragraphs.append(wrapped_paragraph)
    # Join all paragraphs into a single string
    wrapped_text = "\n\n".join(wrapped_paragraphs)
    return wrapped_text


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


async def explain(
    source_code: str, traceback: str, exception: str, really_run=True
) -> None:
    import httpx

    user_prompt = "Explain what the root cause of this error is, given the following source code context for each stack frame and a traceback, and propose a fix. In your response, never refer to the frames given below (as in, 'frame 0'). Instead, always refer only to specific lines and filenames of source code.\n"
    user_prompt += "\n"
    user_prompt += "Source code for each stack frame:\n```\n"
    user_prompt += source_code + "\n```\n"
    user_prompt += traceback + "\n\n"
    user_prompt += "stop reason = " + exception + "\n"
    text = ""

    if not really_run:
        print(user_prompt)
        return

    model = get_model()
    if not model:
        return

    try:
        completion = openai.ChatCompletion.create(
            model=model,
            request_timeout=30,
            messages=[{"role": "user", "content": user_prompt}],
        )
        text = completion.choices[0].message.content
        print(chatdbg_utils.word_wrap_except_code_blocks(text))
    except openai.error.AuthenticationError:
        print(
            "You need a valid OpenAI key to use ChatDBG. You can get a key here: https://openai.com/api/"
        )
        print("Set the environment variable OPENAI_API_KEY to your key value.")
