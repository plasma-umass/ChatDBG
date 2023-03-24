from typing import Tuple, Union
#!env python3
import lldb
import asyncio
# xcrun python3 -m pip install whatever

def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    debugger.HandleCommand('command script add -f chatdbg_lldb.why why')
# From lldbinit

def get_process() -> Union[None, lldb.SBProcess]:
    """
    Get the process that the current target owns.
    :return: An lldb object representing the process (lldb.SBProcess) that this target owns.
    """
    return get_target().process

def get_frame() -> lldb.SBFrame:
    """
    Get the current frame of the running process.

    :return: The current frame of the running process as an SBFrame object.
    """
    # Initialize frame variable to None
    frame = None
    for thread in get_process():
        # Loop through the threads in the process
        if thread.GetStopReason() != lldb.eStopReasonNone and thread.GetStopReason() != lldb.eStopReasonInvalid:
            # If the stop reason is not "none" or "invalid", get the frame at index 0 and break the loop.
            frame = thread.GetFrameAtIndex(0)
            break
    if not frame:
        # If frame is None, print a warning message
        # print("[-] warning: get_frame() failed. Is the target binary started?")
        # The warning message has been commented out, so just pass.
        pass
    # Return the current frame.
    return frame

def get_thread() -> lldb.SBThread:
    """
    Returns the currently stopped thread in the debugged process.
    :return: The currently stopped thread or None if no thread is stopped.
    """
    thread = None
    # Iterate over threads in the process
    for _thread in get_process():
        # Check if thread is stopped for a valid reason
        if _thread.GetStopReason() != lldb.eStopReasonNone and _thread.GetStopReason() != lldb.eStopReasonInvalid:
            thread = _thread
    if not thread:
        # No stopped thread was found
        pass
    return thread

def get_target() -> lldb.SBTarget:
    target = lldb.debugger.GetSelectedTarget()
    if not target:
        return None
    return target
#

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
import re

def get_function_by_line(source_code: str, line_number: int) -> str:
    source_code = re.sub('\\/\\/.*', '', source_code)
    source_code = re.sub('\\/\\*[\\s\\S]*?\\*\\/', '', source_code)
    source_code = re.sub('\\s+', '', source_code)
    function_regex = '(?P<signature>(static|const)?[\\w\\s\\*]+)(?P<name>\\w+)\\((?P<args>[\\w\\s\\*,]*)\\)(?P<body>{[\\s\\S]*?})'
    functions = re.finditer(function_regex, source_code)
    for function in functions:
        function_body = function.group('body')
        line_count = function_body.count('\n')
        print('checking ', function_body)
        if line_number <= line_count:
            return function.group('signature') + function.group('name') + '(' + function.group('args') + ')' + function_body
        line_number -= line_count
    return None

def stackTrace(debugger: any) -> Tuple[str, str, str]:
    import os
    target = get_target()
    if not target:
        return ''
    thread = get_thread()
    if not thread:
        return ''
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        return ''
    frame = thread.GetFrameAtIndex(0)
    stack_trace = ''
    source_code = ''
    for index, frame in enumerate(thread):
        function = frame.GetFunction()
        if not function:
            continue
        func_name = frame.GetFunctionName()
        line_entry = frame.GetLineEntry()
        file_spec = line_entry.GetFileSpec()
        file_name = file_spec.GetFilename()
        directory = file_spec.GetDirectory()
        full_file_name = os.path.join(directory, file_name)
        line_num = line_entry.GetLine()
        col_num = line_entry.GetColumn()
        stack_trace += f'frame {index}: {func_name} at {file_name}:{line_num}:{col_num}\n'
        try:
            source_code += read_lines(full_file_name, line_num - 10, line_num) + '\n'
            source_code += '-' * (col_num - 1) + '^' + '\n\n'
        except:
            # Couldn't find the source for some reason. Skip the file.
            pass
    error_reason = thread.GetStopDescription(255)
    return (source_code, stack_trace, error_reason)
import sys
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

async def explain(source_code: str, traceback: str, exception: str) -> None:
    import httpx
    user_prompt = 'Explain what the root cause of this error is, given the following source code and traceback, and propose a fix.'
    user_prompt += '\n'
    user_prompt += 'source code:\n```\n'
    user_prompt += source_code + '\n```\n'
    user_prompt += traceback + '\n\n'
    user_prompt += 'stop reason = ' + exception + '\n'
    text = ''
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

def why(debugger: lldb.SBDebugger, command: str, result: str, internal_dict: dict) -> None:
    """
    Check if program is attached to a debugger.
    Check if code has been run before executing the `why` command.
    Check if execution stopped at a breakpoint or an error.
    Get stack trace and call `explain` function using asyncio.
    """
    if not get_target():
        # Check if program is attached to a debugger.
        print('Must be attached to a program to ask `why`.')
        return
    # Get the current thread.
    thread = get_thread()
    if not thread:
        # Check if code has been run before executing the `why` command.
        print('Must run the code first to ask `why`.')
        return
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        # Check if execution stopped at a breakpoint or an error.
        print('Execution stopped at a breakpoint, not an error.')
        return
    # Get the current stack trace.
    the_trace = stackTrace(debugger)
    # Call `explain` function with stack trace as arguments.
    asyncio.run(explain(the_trace[0], the_trace[1], the_trace[2]))
