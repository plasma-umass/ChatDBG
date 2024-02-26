import argparse
import os
import sys
import textwrap
from typing import Any, Optional, Tuple

import lldb
import json

import llm_utils
import openai

from assistant.lite_assistant import LiteAssistant
import chatdbg_utils
import clangd_lsp_integration


# The file produced by the panic handler if the Rust program is using the chatdbg crate.
rust_panic_log_filename = "panic_log.txt"


def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    debugger.HandleCommand("settings set prompt '(ChatDBG lldb) '")


def is_debug_build(debugger: lldb.SBDebugger) -> bool:
    """Returns False if not compiled with debug information."""
    target = debugger.GetSelectedTarget()
    if not target:
        return False
    for module in target.module_iter():
        for cu in module.compile_unit_iter():
            for line_entry in cu:
                if line_entry.GetLine() > 0:
                    return True
    return False


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
        file_path = line_entry.GetFileSpec().fullpath
        lineno = line_entry.GetLine()
        col_num = line_entry.GetColumn()

        # If we are in a subdirectory, use a relative path instead.
        if file_path.startswith(os.getcwd()):
            file_path = os.path.relpath(file_path)

        max_line_length = 100

        try:
            (lines, first) = llm_utils.read_lines(file_path, lineno - 10, lineno)
            block = llm_utils.number_group_of_lines(lines, first)

            stack_trace += (
                truncate_string(
                    f'frame {index}: {func_name}({",".join(arg_list)}) at {file_path}:{lineno}:{col_num}',
                    max_line_length - 3,  # 3 accounts for ellipsis
                )
                + "\n"
            )
            if len(var_list) > 0:
                for var in var_list:
                    stack_trace += "  " + truncate_string(var, max_line_length) + "\n"
            source_code += f"/* frame {index} in {file_path} */\n"
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
) -> None:
    """
    The why command is where we use the refined stack trace system.
    We send information once to GPT, and receive an explanation.
    There is a bit of work to determine what context we end up sending to GPT.
    Notably, we send a summary of all stack frames, including locals.
    """
    if not is_debug_build(debugger):
        print(
            "Your program must be compiled with debug information (`-g`) to use `why`."
        )
        sys.exit(1)
    # Check if debugger is attached to a program.
    if not get_target():
        print("Must be attached to a program to ask `why`.")
        sys.exit(1)
    # Check if the program has been run prior to executing the `why` command.
    thread = get_thread()
    if not thread:
        print("Must run the code first to ask `why`.")
        sys.exit(1)
    if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
        print("Execution stopped at a breakpoint, not an error.")
        sys.exit(1)

    the_prompt = buildPrompt(debugger)
    args, _ = chatdbg_utils.parse_known_args(command)
    chatdbg_utils.explain(the_prompt[0], the_prompt[1], the_prompt[2], args)


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


def _capture_onecmd(debugger, cmd):
    # Get the command interpreter from the debugger
    interpreter = debugger.GetCommandInterpreter()

    # Create an object to hold the result of the command execution
    result = lldb.SBCommandReturnObject()

    # Execute a command (e.g., "version" to get the LLDB version)
    interpreter.HandleCommand(cmd, result)

    # Check if the command was executed successfully
    if result.Succeeded():
        # Get the output of the command
        output = result.GetOutput()
        return output
    else:
        # Get the error message if the command failed
        error = result.GetError()
        return f"Command Error: {error}"


def _instructions():
    return textwrap.dedent(
        """
            You are an assistant debugger.
            The user is having an issue with their code, and you are trying to help them find the root cause.
            They will provide a short summary of the issue and a question to be answered.
            Call the `lldb` function to run lldb debugger commands on the stopped program.
            Don't hesitate to use as many function calls as needed to give the best possible answer.
            Once you have identified the root cause of the problem, explain it and provide a way to fix the issue if you can.
        """
    ).strip()


def _make_assistant(debugger: lldb.SBDebugger, args: argparse.Namespace):
    def lldb(command: str) -> str:
        """
        {
            "name": "lldb",
            "description": "Run an LLDB command and get the response.",
            "parameters": {
                "type": "object",
                "properties": {
                    "command": {
                        "type": "string",
                        "description": "The LLDB command to run, possibly with arguments."
                    }
                },
                "required": [ "command" ]
            }
        }
        """
        return _capture_onecmd(debugger, command)

    def get_code_surrounding(filename: str, lineno: int) -> str:
        """
        {
            "name": "get_code_surrounding",
            "description": "Returns the code in the given file surrounding and including the provided line number.",
            "parameters": {
                "type": "object",
                "properties": {
                    "filename": {
                        "type": "string",
                        "description": "The filename to read from."
                    },
                    "lineno": {
                        "type": "integer",
                        "description": "The line number to focus on. Some context before and after that line will be provided."
                    }
                },
                "required": [ "filename", "lineno" ]
            }
        }
        """
        (lines, first) = llm_utils.read_lines(filename, lineno - 7, lineno + 3)
        return llm_utils.number_group_of_lines(lines, first)

    clangd = clangd_lsp_integration.clangd()

    def find_definition(filename: str, lineno: int, character: int) -> str:
        """
        {
            "name": "find_definition",
            "description": "Returns the definition for the symbol at the given source location.",
            "parameters": {
                "type": "object",
                "properties": {
                    "filename": {
                        "type": "string",
                        "description": "The filename the code location is from."
                    },
                    "lineno": {
                        "type": "integer",
                        "description": "The line number where the symbol is present."
                    },
                    "character": {
                        "type": "integer",
                        "description": "The column number where the symbol is present."
                    }
                },
                "required": [ "filename", "lineno", "character" ]
            }
        }
        """
        clangd.didOpen(filename, "c" if filename.endswith(".c") else "cpp")
        definition = clangd.definition(filename, lineno, character)
        clangd.didClose(filename)

        if "result" not in definition or not definition["result"]:
            return "No definition found."

        path = clangd_lsp_integration.uri_to_path(definition["result"][0]["uri"])
        start_lineno = definition["result"][0]["range"]["start"]["line"] + 1
        end_lineno = definition["result"][0]["range"]["end"]["line"] + 1
        (lines, first) = llm_utils.read_lines(path, start_lineno - 5, end_lineno + 5)
        content = llm_utils.number_group_of_lines(lines, first)
        line_string = (
            f"line {start_lineno}"
            if start_lineno == end_lineno
            else f"lines {start_lineno}-{end_lineno}"
        )
        return f"""File '{path}' at {line_string}:\n```\n{content}\n```"""

    assistant = LiteAssistant(
        _instructions(),
        model=args.llm,
        timeout=args.timeout,
        debug=args.debug,
    )

    assistant.add_function(lldb)
    assistant.add_function(get_code_surrounding)

    if not clangd_lsp_integration.is_available():
        print("[WARNING] clangd is not available.")
        print("[WARNING] The `find_definition` function will not be made available.")
    else:
        assistant.add_function(find_definition)

    return assistant


def get_frame_summary() -> str:
    target = lldb.debugger.GetSelectedTarget()
    if not target or not target.process:
        return None

    for thread in target.process:
        reason = thread.GetStopReason()
        if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
            break

    summaries = []
    for i, frame in enumerate(thread):
        name = frame.GetDisplayFunctionName().split("(")[0]
        arguments = []
        for j in range(
            frame.GetFunction().GetType().GetFunctionArgumentTypes().GetSize()
        ):
            arg = frame.FindVariable(frame.GetFunction().GetArgumentName(j))
            if not arg:
                continue
            arguments.append(f"{arg.GetName()}={arg.GetValue()}")

        line_entry = frame.GetLineEntry()
        file_path = line_entry.GetFileSpec().fullpath
        lineno = line_entry.GetLine()

        # If we are in a subdirectory, use a relative path instead.
        if file_path.startswith(os.getcwd()):
            file_path = os.path.relpath(file_path)

        # Skip frames for which we have no source -- likely system frames.
        if not os.path.exists(file_path):
            continue

        summaries.append(f"{i}: {name}({', '.join(arguments)}) at {file_path}:{lineno}")
    return "\n".join(reversed(summaries))


def get_error_message() -> Optional[str]:
    target = lldb.debugger.GetSelectedTarget()
    if not target or not target.process:
        return None

    for thread in target.process:
        reason = thread.GetStopReason()
        if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
            break

    return thread.GetStopDescription(1024)


@lldb.command("chat")
def chat(debugger: lldb.SBDebugger, command: str, result: str, internal_dict: dict):
    args, remaining = chatdbg_utils.parse_known_args(command.split())
    assistant = _make_assistant(debugger, args)

    prompt = f"""Here is the reason the program stopped execution:
```
{get_error_message()}
```

Here is a summary of the stack frames, omitting those not associated with source code:
```
{get_frame_summary()}
```

{" ".join(remaining) if remaining else "What's the problem?"}"""

    assistant.run(prompt)
