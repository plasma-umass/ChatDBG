#!env python3
from typing import Tuple, Union
import lldb
import asyncio

import utils

def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    # Install the `why` command.
    debugger.HandleCommand('command script add -f chatdbg_lldb.why why')
    # Update the prompt.
    debugger.HandleCommand("settings set prompt '(ChatDBG lldb) '")

    
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

def buildPrompt(debugger: any) -> Tuple[str, str, str]:
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
    
    # magic number - don't bother walking up more than this many frames.
    # This is just to prevent overwhelming OpenAI (or to cope with a stack overflow!).
    max_frames = 10
    
    for index, frame in enumerate(thread):
        if index >= max_frames:
            break
        function = frame.GetFunction()
        if not function:
            continue
        full_func_name = frame.GetFunctionName()
        func_name = full_func_name.split('(')[0]
        arg_list = []

        # Build up an array of argument values to the function.
        for i in range(len(frame.GetFunction().GetType().GetFunctionArgumentTypes())):
            arg_list.append(str(frame.FindVariable(frame.GetFunction().GetArgumentName(i))).split('=')[1].strip())
            
        line_entry = frame.GetLineEntry()
        file_spec = line_entry.GetFileSpec()
        file_name = file_spec.GetFilename()
        directory = file_spec.GetDirectory()
        full_file_name = os.path.join(directory, file_name)
        line_num = line_entry.GetLine()
        col_num = line_entry.GetColumn()
        stack_trace += f'frame {index}: {func_name}({",".join(arg_list)}) at {file_name}:{line_num}:{col_num}\n'
        try:
            source_code += f'/* frame {index} */\n'
            source_code += utils.read_lines(full_file_name, line_num - 10, line_num) + '\n'
            source_code += '-' * (col_num - 1) + '^' + '\n\n'
        except:
            # Couldn't find the source for some reason. Skip the file.
            pass
    error_reason = thread.GetStopDescription(255)
    return (source_code, stack_trace, error_reason)

def why(debugger: lldb.SBDebugger, command: str, result: str, internal_dict: dict) -> None:
    """
    Check if program is attached to a debugger.
    Check if code has been run before executing the `why` command.
    Check if execution stopped at a breakpoint or an error.
    Get source code, stack trace, and exception, and call `explain` function using asyncio.
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
    the_prompt = buildPrompt(debugger)
    # Call `explain` function with pieces of the_prompt  as arguments.
    asyncio.run(utils.explain(the_prompt[0], the_prompt[1], the_prompt[2]))
