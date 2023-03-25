# Add 'source <path to chatdbg>/chatdbg_gdb.py' to ~/.gdbinit

import asyncio
import gdb
import os
import openai
import openai_async
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


    try:
        completion = await openai_async.chat_complete(openai.api_key, timeout=30, payload={'model': 'gpt-3.5-turbo', 'messages': [{'role': 'user', 'content': user_prompt}]})
        json_payload = completion.json()
        text = json_payload['choices'][0]['message']['content']
    except (openai.error.AuthenticationError, httpx.LocalProtocolError):
        print()
        print('You need an OpenAI key to use commentator. You can get a key here: https://openai.com/api/')
        print('Set the environment variable OPENAI_API_KEY to your key value.')
        import sys
        sys.exit(1)
    except Exception as e:
        print(f'EXCEPTION {e}')
        pass
    print(word_wrap_except_code_blocks(text))



def read_lines_list(file_path: str, start_line: int, end_line: int) -> [str]:
    """
    Read lines from a file and return a list containing the lines between start_line and end_line.

    Args:
        file_path (str): The path of the file to read.
        start_line (int): The line number of the first line to include (1-indexed).
        end_line (int): The line number of the last line to include.

    Returns:
        [str]: A list of the lines between start_line and end_line.

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
    # return the requested lines as a list
    return lines[start_line:end_line]

# Set the prompt to gdb-ChatDBG
gdb.prompt_hook = lambda x: "(gdb-ChatDBG) "

last_error_type = ""

def stop_handler(event):
    """Sets last error type so we can report it later."""
    # Check if the event is a stop event
    global last_error_type
    if not hasattr(event, 'stop_signal'):
        last_error_type = "" # Not a real error (e.g., a breakpoint)
        return
    if event.stop_signal is not None:
        last_error_type = event.stop_signal

gdb.events.stop.connect(stop_handler)

        
# Implement the command `why`
class Why(gdb.Command):

    def __init__(self):
        gdb.Command.__init__(self, "why", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        try:
            frame = gdb.selected_frame()
        except:
            print("Must run the code first to ask `why`.")
            return
        if not last_error_type:
            print("Execution stopped at a breakpoint, not an error.")
            return
        the_prompt = buildPrompt()
        #print(the_prompt[0])
        #print(the_prompt[1])
        #print(the_prompt[2])
        # Call `explain` function with pieces of the_prompt  as arguments.
        asyncio.run(explain(the_prompt[0], the_prompt[1], the_prompt[2]))
        
Why()

def buildPrompt() -> str:
    thread = gdb.selected_thread()
    if not thread:
        return ''

    stack_trace = ""
    source_code = ""

    frames = []
    frame = gdb.selected_frame()
    
    # Walk the stack and build up the frames list.
    while frame is not None:
        func_name = frame.name()
        symtab_and_line = frame.find_sal()
        if symtab_and_line.symtab is not None:
            filename = symtab_and_line.symtab.filename
        else:
            filename = None
        if symtab_and_line.line is not None:
            print(dir(symtab_and_line))
            lineno = symtab_and_line.line
            colno = None
        else:
            lineno = None
            colno = None
        args = []
        block = frame.block()
        for symbol in block:
            if symbol.is_argument:
                name = symbol.name
                value = frame.read_var(name)
                args.append((name, value))
        frames.append((filename, func_name, args, lineno, colno))
        frame = frame.older()

    # Now build the stack trace and source code strings.
    for i, frame_info in enumerate(frames):
        file_name = frame_info[0]
        func_name = frame_info[1]
        line_num = frame_info[3]
        arg_list = []
        for arg in frame_info[2]:
            arg_list.append(str(arg[1])) # Note: arg[0] is the name of the argument
        stack_trace += f'frame {i}: {func_name}({",".join(arg_list)}) at {file_name}:{line_num}\n'
        try:
            source_code += f'/* frame {i} */\n'
            lines = read_lines_list(file_name, line_num - 10, line_num)
            source_code += '\n'.join(lines) + '\n'
            # Get the spaces before the last line.
            num_spaces = len(lines[-1]) - len(lines[-1].lstrip())
            source_code += ' ' * num_spaces + '^' + '-' * (79 - num_spaces) + '\n'
        except:
            # Couldn't find source for some reason. Skip file.
            pass

    return (source_code, stack_trace, last_error_type)


