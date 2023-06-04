#!env python3
import lldb
import asyncio
import re
import utils

from typing import Tuple, Union

def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    # Update the prompt.
    debugger.HandleCommand("settings set prompt '(ChatDBG lldb) '")

def is_debug_build(debugger, command, result, internal_dict) -> bool:
    """Returns False if not compiled with debug information."""
    target = debugger.GetSelectedTarget()
    if target:
        module = target.GetModuleAtIndex(0)
        if module:
            compile_unit = module.GetCompileUnitAtIndex(0)
            if compile_unit.IsValid():
                return True
    return False

   
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

def truncate_string(string, n):
    if len(string) <= n:
        return string
    else:
        return string[:n] + "..."

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

    index = 0
    for frame in thread:
        if index >= max_frames:
            break
        function = frame.GetFunction()
        if not function:
            continue
        full_func_name = frame.GetFunctionName()
        func_name = full_func_name.split('(')[0]
        arg_list = []
        type_list = []

        # Build up an array of argument values to the function, with type info.
        for i in range(len(frame.GetFunction().GetType().GetFunctionArgumentTypes())):
            arg = frame.FindVariable(frame.GetFunction().GetArgumentName(i))
            arg_name = str(arg).split('=')[0].strip()
            arg_val = str(arg).split('=')[1].strip()
            arg_list.append(f"{arg_name} = {arg_val}")
            
        # Get the frame variables
        variables = frame.GetVariables(True, True, True, True)
        var_list = []
        
        for var in variables:
            name = var.GetName()
            value = var.GetValue()
            type = var.GetTypeName()
            # Check if the value is a pointer
            if var.GetType().IsPointerType():
                # Attempt to dereference the pointer
                try:
                    deref_value = var.Dereference().GetValue()
                    var_list.append(f"{type} {name} = {value} (*{name} = {deref_value})")
                except:
                    var_list.append(f"{type} {name} = {value}")
            
        line_entry = frame.GetLineEntry()
        file_spec = line_entry.GetFileSpec()
        file_name = file_spec.GetFilename()
        directory = file_spec.GetDirectory()
        full_file_name = os.path.join(directory, file_name)
        line_num = line_entry.GetLine()
        col_num = line_entry.GetColumn()

        max_line_length = 100
        
        stack_trace += truncate_string(f'frame {index}: {func_name}({",".join(arg_list)}) at {file_name}:{line_num}:{col_num}\n', max_line_length - 3) + '\n' # 3 accounts for ellipsis
        stack_trace += "Local variables: " + truncate_string(','.join(var_list), max_line_length) + '\n'
        try:
            source_code += f'/* frame {index} in {file_name} */\n'
            source_code += utils.read_lines(full_file_name, line_num - 10, line_num) + '\n'
            source_code += '-' * (utils.read_lines_width() + col_num - 1) + '^' + '\n\n'
            index += 1
        except:
            # Couldn't find the source for some reason. Skip the file.
            pass
    error_reason = thread.GetStopDescription(255)
    return (source_code, stack_trace, error_reason)

@lldb.command("why")
def why(debugger: lldb.SBDebugger, command: str, result: str, internal_dict: dict, really_run = True) -> None:
    """
    Root cause analysis for an error.
    """
    # Check if there is debug info.
    if not is_debug_build(debugger, command, result, internal_dict):
        print('Your program must be compiled with debug information (`-g`) to use `why`.')
        return
    # Check if program is attached to a debugger.
    if not get_target():
        print('Must be attached to a program to ask `why`.')
        return
    # Check if code has been run before executing the `why` command.
    thread = get_thread()
    if not thread:
        print('Must run the code first to ask `why`.')
        return
    # Check why code stopped running.
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        # Check if execution stopped at a breakpoint or an error.
        print('Execution stopped at a breakpoint, not an error.')
        return
    the_prompt = buildPrompt(debugger)
    asyncio.run(utils.explain(the_prompt[0], the_prompt[1], the_prompt[2], really_run))

@lldb.command("why_prompt")
def why_prompt(debugger: lldb.SBDebugger, command: str, result: str, internal_dict: dict) -> None:
    why(debugger, command, result, internal_dict, really_run=False)
    
