#!env python3
import lldb
import asyncio

# xcrun python3 -m pip install whatever


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand("command script add -f chatdbg_lldb.why why")


# From lldbinit
def get_process():
    """
    A read only property that returns an lldb object
    that represents the process (lldb.SBProcess)that this target owns.
    """
    return get_target().process


def get_frame():
    frame = None
    # SBProcess supports thread iteration -> SBThread
    for thread in get_process():
        if (thread.GetStopReason() != lldb.eStopReasonNone) and (
            thread.GetStopReason() != lldb.eStopReasonInvalid
        ):
            frame = thread.GetFrameAtIndex(0)
            break
    # this will generate a false positive when we start the target the first time because there's no context yet.
    if not frame:
        # print("[-] warning: get_frame() failed. Is the target binary started?")
        pass
    return frame


def get_thread():
    thread = None
    # SBProcess supports thread iteration -> SBThread
    for _thread in get_process():
        if (_thread.GetStopReason() != lldb.eStopReasonNone) and (
            _thread.GetStopReason() != lldb.eStopReasonInvalid
        ):
            thread = _thread

    if not thread:
        # print("[-] warning: get_thread() failed. Is the target binary started?")
        pass

    return thread


def get_target():
    target = lldb.debugger.GetSelectedTarget()
    if not target:
        # print("[-] error: no target available. please add a target to lldb.")
        return None
    return target


#
def read_lines(file_path, start_line, end_line):
    with open(file_path, "r") as f:
        lines = f.readlines()
        lines = [line.rstrip() for line in lines]  # remove trailing newline characters

    start_line = max(0, start_line - 1)  # convert to 0-based indexing
    end_line = min(len(lines), end_line)  # ensure end_line is within range

    return "\n".join(lines[start_line:end_line])


import re


def get_function_by_line(source_code, line_number):
    """
    Given a source code and a line number, this function returns the function that contains the line number.
    """
    # Remove all single and multi-line comments
    source_code = re.sub(r"\/\/.*", "", source_code)
    source_code = re.sub(r"\/\*[\s\S]*?\*\/", "", source_code)

    # Remove all whitespaces and newlines
    source_code = re.sub(r"\s+", "", source_code)

    # Find all function definitions
    function_regex = r"(?P<signature>(static|const)?[\w\s\*]+)(?P<name>\w+)\((?P<args>[\w\s\*,]*)\)(?P<body>{[\s\S]*?})"
    functions = re.finditer(function_regex, source_code)

    # Find the function that contains the line number
    for function in functions:
        function_body = function.group("body")
        line_count = function_body.count("\n")

        print("checking ", function_body)
        if line_number <= line_count:
            return (
                function.group("signature")
                + function.group("name")
                + "("
                + function.group("args")
                + ")"
                + function_body
            )

        line_number -= line_count

    return None


def stackTrace(debugger):
    import os

    target = get_target()
    if not target:
        return ""
    thread = get_thread()
    if not thread:
        # Not running
        return ""

    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        return ""

    frame = thread.GetFrameAtIndex(0)
    stack_trace = ""
    source_code = ""
    for index, frame in enumerate(thread):
        function = frame.GetFunction()
        if not function:
            # No debug info
            continue

        # Check for debugging info https://lldb.llvm.org/python_api/lldb.SBFunction.html
        func_name = frame.GetFunctionName()
        line_entry = frame.GetLineEntry()
        file_spec = line_entry.GetFileSpec()
        file_name = file_spec.GetFilename()
        directory = file_spec.GetDirectory()
        full_file_name = os.path.join(directory, file_name)
        line_num = line_entry.GetLine()
        col_num = line_entry.GetColumn()
        stack_trace += (
            f"frame {index}: {func_name} at {file_name}:{line_num}:{col_num}\n"
        )
        source_code += read_lines(full_file_name, line_num - 10, line_num) + "\n"
        source_code += "-" * (col_num - 1) + "^" + "\n\n"

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


async def explain(source_code, traceback, exception):
    import httpx

    user_prompt = "Explain what the root cause of this error is, given the following source code and traceback, and propose a fix."
    user_prompt += "\n"
    user_prompt += "source code:\n```\n"
    user_prompt += source_code + "\n```\n"

    user_prompt += traceback + "\n\n"

    user_prompt += "stop reason = " + exception + "\n"

    text = ""
    try:
        completion = await openai_async.chat_complete(
            openai.api_key,
            timeout=30,
            payload={
                "model": "gpt-3.5-turbo",
                "messages": [{"role": "user", "content": user_prompt}],
            },
        )
        json_payload = completion.json()
        text = json_payload["choices"][0]["message"]["content"]
    except (openai.error.AuthenticationError, httpx.LocalProtocolError):
        print()
        print(
            "You need an OpenAI key to use commentator. You can get a key here: https://openai.com/api/"
        )
        print("Set the environment variable OPENAI_API_KEY to your key value.")
        import sys

        sys.exit(1)
    except Exception as e:
        print(f"EXCEPTION {e}")
        pass
    print(word_wrap_except_code_blocks(text))


def why(debugger, command, result, internal_dict):
    if not get_target():
        print("Must be attached to a program to ask `why`.")
        return
    thread = get_thread()
    if not thread:
        print("Must run the code first to ask `why`.")
        return
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        print("Execution stopped at a breakpoint, not an error.")
        return

    the_trace = stackTrace(debugger)
    asyncio.run(explain(the_trace[0], the_trace[1], the_trace[2]))
