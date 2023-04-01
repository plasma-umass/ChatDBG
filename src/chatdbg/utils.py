import openai
import openai_async
import os
import sys
import textwrap

def word_wrap_except_code_blocks(text: str) -> str:
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
    paragraphs = text.split('\n\n')
    wrapped_paragraphs = []
    # Check if currently in a code block.
    in_code_block = False
    # Loop through each paragraph and apply appropriate wrapping.
    for paragraph in paragraphs:
        # If this paragraph starts and ends with a code block, add it as is.
        if paragraph.startswith('```') and paragraph.endswith('```'):
            wrapped_paragraphs.append(paragraph)
            continue
        # If this is the beginning of a code block add it as is.
        if paragraph.startswith('```'):
            in_code_block = True
            wrapped_paragraphs.append(paragraph)
            continue
        # If this is the end of a code block stop skipping text.
        if paragraph.endswith('```'):
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
    wrapped_text = '\n\n'.join(wrapped_paragraphs)
    return wrapped_text

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
    with open(file_path, 'r') as f:
        # read all the lines from the file
        lines = f.readlines()
        # remove trailing newline characters
        lines = [line.rstrip() for line in lines]
    # convert start_line to 0-based indexing
    start_line = max(0, start_line - 1)
    # ensure end_line is within range
    end_line = min(len(lines), end_line)
    # return the requested lines as a string
    return '\n'.join(lines[start_line:end_line])

async def explain(source_code: str, traceback: str, exception: str) -> None:
    import httpx
    user_prompt = 'Explain what the root cause of this error is, given the following source code and traceback, and propose a fix.'
    user_prompt += '\n'
    user_prompt += 'source code:\n```\n'
    user_prompt += source_code + '\n```\n'
    user_prompt += traceback + '\n\n'
    user_prompt += 'stop reason = ' + exception + '\n'
    text = ''
    
    # uncomment to view prompt and not send to OpenAI
    #print(user_prompt)
    #return

    if not 'OPENAI_API_KEY' in os.environ:
        print('You need a valid OpenAI key to use ChatDBG. You can get a key here: https://openai.com/api/')
        print('Set the environment variable OPENAI_API_KEY to your key value.')
        return
    
    try:
        completion = await openai_async.chat_complete(openai.api_key, timeout=30, payload={'model': 'gpt-3.5-turbo', 'messages': [{'role': 'user', 'content': user_prompt}]})
        json_payload = completion.json()
        text = json_payload['choices'][0]['message']['content']
    except (openai.error.AuthenticationError, httpx.LocalProtocolError, KeyError):
        # Something went wrong.
        print()
        print('You need a valid OpenAI key to use ChatDBG. You can get a key here: https://openai.com/api/')
        print('Set the environment variable OPENAI_API_KEY to your key value.')
        import sys
        sys.exit(1)
    except Exception as e:
        print(f'EXCEPTION {e}, {type(e)}')
        pass
    print(word_wrap_except_code_blocks(text))

