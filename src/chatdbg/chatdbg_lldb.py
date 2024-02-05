import os
import pathlib
import sys
from typing import Any, Optional, Tuple

import lldb
import json

import llm_utils
import subprocess
import openai


sys.path.append(os.path.abspath(pathlib.Path(__file__).parent.resolve()))
import chatdbg_utils
import conversation


# The file produced by the panic handler if the Rust program is using the chatdbg crate.
rust_panic_log_filename = "panic_log.txt"


def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    # Update the prompt.
    debugger.HandleCommand("settings set prompt '(ChatDBG lldb) '")


def is_debug_build(debugger, command, result, internal_dict) -> bool:
    """Returns False if not compiled with debug information."""
    target = debugger.GetSelectedTarget()
    if not target:
        return False

    has_debug_symbols = False
    for module in target.module_iter():
        for cu in module.compile_unit_iter():
            for line_entry in cu:
                if line_entry.GetLine() > 0:
                    has_debug_symbols = True
                    break
    return has_debug_symbols


def is_debug_build_prev(debugger, command, result, internal_dict) -> bool:
    target = debugger.GetSelectedTarget()
    if target:
        module = target.GetModuleAtIndex(0)
        if module:
            compile_unit = module.GetCompileUnitAtIndex(0)
            if compile_unit.IsValid():
                return True
    return False


# From lldbinit


def get_process() -> Optional[lldb.SBProcess]:
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
        if (
            thread.GetStopReason() != lldb.eStopReasonNone
            and thread.GetStopReason() != lldb.eStopReasonInvalid
        ):
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
        if (
            _thread.GetStopReason() != lldb.eStopReasonNone
            and _thread.GetStopReason() != lldb.eStopReasonInvalid
        ):
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


def buildPrompt(debugger: Any) -> Tuple[str, str, str]:
    target = get_target()
    if not target:
        return ("", "", "")
    thread = get_thread()
    if not thread:
        return ("", "", "")
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        return ("", "", "")
    frame = thread.GetFrameAtIndex(0)
    stack_trace = ""
    source_code = ""

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
        func_name = full_func_name.split("(")[0]
        arg_list = []

        # Build up an array of argument values to the function, with type info.
        for i in range(len(frame.GetFunction().GetType().GetFunctionArgumentTypes())):
            arg = frame.FindVariable(frame.GetFunction().GetArgumentName(i))
            if not arg:
                continue
            arg_name = str(arg).split("=")[0].strip()
            arg_val = str(arg).split("=")[1].strip()
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
                    var_list.append(
                        f"{type} {name} = {value} (*{name} = {deref_value})"
                    )
                except:
                    var_list.append(f"{type} {name} = {value}")

        line_entry = frame.GetLineEntry()
        file_spec = line_entry.GetFileSpec()
        file_name = file_spec.GetFilename()
        directory = file_spec.GetDirectory()
        full_file_name = os.path.join(directory, file_name)
        lineno = line_entry.GetLine()
        col_num = line_entry.GetColumn()

        max_line_length = 100

        try:
            (lines, first) = llm_utils.read_lines(full_file_name, lineno - 10, lineno)
            block = llm_utils.number_group_of_lines(lines, first)

            stack_trace += (
                truncate_string(
                    f'frame {index}: {func_name}({",".join(arg_list)}) at {file_name}:{lineno}:{col_num}',
                    max_line_length - 3,
                )
                + "\n"
            )  # 3 accounts for ellipsis
            if len(var_list) > 0:
                stack_trace += (
                    "Local variables: "
                    + truncate_string(",".join(var_list), max_line_length)
                    + "\n"
                )
            source_code += f"/* frame {index} in {file_name} */\n"
            source_code += block + "\n\n"
        except:
            # Couldn't find the source for some reason. Skip the file.
            continue
        index += 1
    error_reason = thread.GetStopDescription(255)
    # If the Rust panic log exists, append it to the error reason.
    try:
        with open(rust_panic_log_filename, "r") as log:
            panic_log = log.read()
        error_reason = panic_log + "\n" + error_reason
    except:
        pass
    return (source_code.strip(), stack_trace.strip(), error_reason.strip())


@lldb.command("why")
def why(
    debugger: lldb.SBDebugger,
    command: str,
    result: str,
    internal_dict: dict,
    really_run=True,
) -> None:
    """
    Root cause analysis for an error.
    """
    # Check if there is debug info.
    if not is_debug_build(debugger, command, result, internal_dict):
        print(
            "Your program must be compiled with debug information (`-g`) to use `why`."
        )
        return
    # Check if program is attached to a debugger.
    if not get_target():
        print("Must be attached to a program to ask `why`.")
        return
    # Check if code has been run before executing the `why` command.
    thread = get_thread()
    if not thread:
        print("Must run the code first to ask `why`.")
        return
    # Check why code stopped running.
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        # Check if execution stopped at a breakpoint or an error.
        print("Execution stopped at a breakpoint, not an error.")
        return
    the_prompt = buildPrompt(debugger)
    chatdbg_utils.explain(the_prompt[0], the_prompt[1], the_prompt[2], really_run)


@lldb.command("why_prompt")
def why_prompt(
    debugger: lldb.SBDebugger, command: str, result: str, internal_dict: dict
) -> None:
    """Output the prompt that `why` would generate (for debugging purposes only)."""
    why(debugger, command, result, internal_dict, really_run=False)


@lldb.command("print-test")
def print_test(
    debugger: lldb.SBDebugger,
    command: str,
    result: str,
    internal_dict: dict,
) -> None:
    """print all variables in a run while recursing through pointers, keeping track of seen addresses"""
    args = command.split()
    recurse_max = 3
    help_string = "Usage: print-test [recurse_max]\n\nrecurse_max: The maximum number of times to recurse through nested structs or pointers to pointers. Default: 3"
    if len(args) > 1 or (len(args) == 1 and args[0] == "--help"):
        print(help_string)
        return
    elif len(args) == 1:
        try:
            recurse_max = int(args[0])
        except ValueError as e:
            print("recurse_max value could not be parsed: %s\n" % args[0])
            return
        if recurse_max < 1:
            print("recurse_max value must be at least 1.\n")
            return
    frame = (
        lldb.debugger.GetSelectedTarget()
        .GetProcess()
        .GetSelectedThread()
        .GetSelectedFrame()
    )

    all_vars = []
    addresses = {}
    for var in frame.get_all_variables():
        # Returns python dictionary for each variable, converts to JSON
        variable = _val_to_json(
            debugger, command, result, internal_dict, var, recurse_max, addresses
        )
        js = json.dumps(variable, indent=4)
        all_vars.append(js)

    # Print all addresses and JSON objects
    # print(addresses)
    for j in all_vars:
        print(j)
    return


def _val_to_json(
    debugger: lldb.SBDebugger,
    command: str,
    result: str,
    internal_dict: dict,
    var: lldb.SBValue,
    recurse_max: int,
    address_book: dict,
) -> dict:
    # Store address
    address_book.setdefault(str(var.GetAddress()), var.GetName())

    json = {}
    json["name"] = var.GetName()
    json["type"] = var.GetTypeName()
    # Dereference pointers
    if "*" in var.GetType().GetName():
        if var.GetValueAsUnsigned() != 0:
            value = "->"
            try:
                deref_val = var.Dereference()
                # If dereferenced value is "seen", then get name from address book
                if str(deref_val.GetAddress()) in address_book:
                    json["value"] = address_book[str(deref_val.GetAddress())]
                else:
                    # Recurse up to max_recurse times
                    for i in range(recurse_max - 1):
                        if "*" in deref_val.GetType().GetName():
                            value += "->"
                            deref_val = deref_val.Dereference()
                        elif len(deref_val.GetType().get_fields_array()) > 0:
                            value = _val_to_json(
                                debugger,
                                command,
                                result,
                                internal_dict,
                                deref_val,
                                recurse_max - i - 1,
                                address_book,
                            )
                            break
                        else:
                            break
                    # Append to -> string or not, depending on type of value
                    if isinstance(value, dict):
                        json["value"] = value
                    else:
                        json["value"] = (
                            value + str(deref_val)[str(deref_val).find("= ") + 2 :]
                        )
            except Exception as e:
                json["value"] = value + "Exception"
        else:
            json["value"] = "nullptr"
    # Recurse through struct fields
    elif len(var.GetType().get_fields_array()) > 0:
        fields = []
        for i in range(var.GetNumChildren()):
            f = var.GetChildAtIndex(i)
            fields.append(
                _val_to_json(
                    debugger,
                    command,
                    result,
                    internal_dict,
                    f,
                    recurse_max - 1,
                    address_book,
                )
            )
        json["value"] = fields
    else:
        json["value"] = str(var)[str(var).find("= ") + 2 :]
    return json


_DEFAULT_FALLBACK_MODELS = ["gpt-4", "gpt-3.5-turbo"]


@lldb.command("converse")
def converse(
    debugger: lldb.SBDebugger,
    command: str,
    result: str,
    internal_dict: dict,
) -> None:
    # Perform typical "why" checks
    # Check if there is debug info.
    if not is_debug_build(debugger, command, result, internal_dict):
        print(
            "Your program must be compiled with debug information (`-g`) to use `converse`."
        )
        return
    # Check if program is attached to a debugger.
    if not get_target():
        print("Must be attached to a program to ask `converse`.")
        return
    # Check if code has been run before executing the `why` command.
    thread = get_thread()
    if not thread:
        print("Must run the code first to ask `converse`.")
        return
    # Check why code stopped running.
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        # Check if execution stopped at a breakpoint or an error.
        print("Execution stopped at a breakpoint, not an error.")
        return

    args = chatdbg_utils.use_argparse(command.split())

    try:
        client = openai.OpenAI(timeout=args.timeout)
    except openai.OpenAIError:
        print("You need an OpenAI key to use this tool.")
        print("You can get a key here: https://platform.openai.com/api-keys")
        print("Set the environment variable OPENAI_API_KEY to your key value.")
        sys.exit(1)

    the_prompt = buildPrompt(debugger)


    if args.show_prompt:
        print("===================== Prompt =====================")
        conversation.converse(client, args, the_prompt[1])
        print("==================================================")
        sys.exit(0)

    print("==================================================")
    print("ChatDBG")
    print("==================================================")
    print(conversation.converse(client, args, the_prompt[1]))
    print("==================================================")

    sys.exit(0)
