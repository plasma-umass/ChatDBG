import argparse
import os
import json
import textwrap
from typing import Any, List, Optional, Tuple, Union

import lldb

import llm_utils

from assistant.lite_assistant import LiteAssistant
import chatdbg_utils
import clangd_lsp_integration


# The file produced by the panic handler if the Rust program is using the chatdbg crate.
RUST_PANIC_LOG_FILENAME = "panic_log.txt"
PROMPT = "(ChatDBG lldb)"


def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    debugger.HandleCommand(f"settings set prompt '{PROMPT} '")


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


def get_process(debugger) -> Optional[lldb.SBProcess]:
    """
    Get the process that the current target owns.
    :return: An lldb object representing the process (lldb.SBProcess) that this target owns.
    """
    target = debugger.GetSelectedTarget()
    return target.process if target else None


def get_thread(debugger: lldb.SBDebugger) -> Optional[lldb.SBThread]:
    """
    Returns a currently stopped thread in the debugged process.
    :return: A currently stopped thread or None if no thread is stopped.
    """
    process = get_process(debugger)
    if not process:
        return None
    for thread in process:
        reason = thread.GetStopReason()
        if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
            return thread
    return thread


def truncate_string(string, n):
    if len(string) <= n:
        return string
    else:
        return string[:n] + "..."


def buildPrompt(debugger: Any) -> Tuple[str, str, str]:
    target = debugger.GetSelectedTarget()
    if not target:
        return ("", "", "")
    thread = get_thread(debugger)
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
        with open(RUST_PANIC_LOG_FILENAME, "r") as log:
            panic_log = log.read()
        error_reason = panic_log + "\n" + error_reason
    except:
        pass
    return (source_code.strip(), stack_trace.strip(), error_reason.strip())


@lldb.command("why")
def why(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    """
    The why command is where we use the refined stack trace system.
    We send information once to GPT, and receive an explanation.
    There is a bit of work to determine what context we end up sending to GPT.
    Notably, we send a summary of all stack frames, including locals.
    """
    if not debugger.GetSelectedTarget():
        result.SetError("must be attached to a program to ask `why`.")
        return
    if not is_debug_build(debugger):
        result.SetError(
            "your program must be compiled with debug information (`-g`) to use `why`."
        )
        return
    thread = get_thread(debugger)
    if not thread:
        result.SetError("must run the code first to ask `why`.")
        return

    the_prompt = buildPrompt(debugger)
    args, _ = chatdbg_utils.parse_known_args(command.split())
    chatdbg_utils.explain(
        the_prompt[0],
        the_prompt[1],
        the_prompt[2],
        args,
        result.AppendMessage,
        result.AppendWarning,
        result.SetError,
    )


@lldb.command("print-test")
def print_test(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
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
    result: lldb.SBCommandReturnObject,
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
    interpreter = debugger.GetCommandInterpreter()
    result = lldb.SBCommandReturnObject()
    interpreter.HandleCommand(cmd, result)

    if result.Succeeded():
        return result.GetOutput()
    else:
        return result.GetError()


def _instructions():
    return textwrap.dedent(
        """
            You are an assistant debugger.
            The user is having an issue with their code, and you are trying to help them find the root cause.
            They will provide a short summary of the issue and a question to be answered.

            Call the `debug` function to run lldb debugger commands on the stopped program.
            Call the `get_code_surrounding` function to retrieve user code and give more context back to the user on their problem.
            Call the `find_definition` function to retrieve the definition of a particular symbol.
            You should call `find_definition` on every symbol that could be linked to the issue.

            Don't hesitate to use as many function calls as needed to give the best possible answer.
            Once you have identified the root cause of the problem, explain it and provide a way to fix the issue if you can.
        """
    ).strip()


@lldb.command("debug")
def _function_lldb(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    result.AppendMessage(_capture_onecmd(debugger, command))


@lldb.command("code")
def _function_code(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    parts = command.split(":")
    if len(parts) != 2:
        result.SetError("usage: code <filename>:<lineno>")
        return
    filename, lineno = parts[0], int(parts[1])
    (lines, first) = llm_utils.read_lines(filename, lineno - 7, lineno + 3)
    formatted = llm_utils.number_group_of_lines(lines, first)
    result.AppendMessage(formatted)


_clangd = None
if clangd_lsp_integration.is_available():
    _clangd = clangd_lsp_integration.clangd()


@lldb.command("definition")
def _function_definition(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    if not clangd_lsp_integration.is_available():
        result.SetError(
            "`clangd` was not found. The `definition` function will not be made available."
        )
        return
    last_space_index = command.rfind(" ")
    if last_space_index == -1:
        result.SetError(
            "`clangd` was not found. The `definition` function will not be made available."
        )
        return
    filename_lineno = command[:last_space_index]
    symbol = command[last_space_index + 1 :]
    parts = filename_lineno.split(":")
    if len(parts) != 2:
        result.SetError("usage: definition <filename>:<lineno> <symbol>")
        return
    filename, lineno = parts[0], int(parts[1])

    # We just return the first match here. Maybe we should find all definitions.
    with open(filename, "r") as file:
        lines = file.readlines()
        if lineno - 1 >= len(lines):
            result.SetError("symbol not found at that location.")
            return
        character = lines[lineno - 1].find(symbol)
        if character == -1:
            result.SetError("symbol not found at that location.")
            return
    global _clangd
    _clangd.didOpen(filename, "c" if filename.endswith(".c") else "cpp")
    definition = _clangd.definition(filename, lineno, character + 1)
    _clangd.didClose(filename)

    if "result" not in definition or not definition["result"]:
        result.SetError("No definition found.")
        return

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
    result.AppendMessage(f"""File '{path}' at {line_string}:\n```\n{content}\n```""")


def _make_assistant(
    debugger: lldb.SBDebugger,
    args: argparse.Namespace,
    result: lldb.SBCommandReturnObject,
) -> LiteAssistant:
    def llm_debug(command: str) -> str:
        """
        {
            "name": "debug",
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
        return _capture_onecmd(debugger, f"debug {command}")

    def llm_get_code_surrounding(filename: str, lineno: int) -> str:
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
        return _capture_onecmd(debugger, f"code {filename}:{lineno}")

    assistant = LiteAssistant(
        _instructions(),
        model=args.llm,
        timeout=args.timeout,
        max_result_tokens=args.tool_call_max_result_tokens,
        debug=args.debug,
    )

    assistant.add_function(llm_debug)
    assistant.add_function(llm_get_code_surrounding)

    if not clangd_lsp_integration.is_available():
        result.AppendWarning(
            "`clangd` was not found. The `find_definition` function will not be made available."
        )
    else:

        def llm_find_definition(filename: str, lineno: int, symbol: str) -> str:
            """
            {
                "name": "find_definition",
                "description": "Returns the definition for the given symbol at the given source line number.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "filename": {
                            "type": "string",
                            "description": "The filename the symbol is from."
                        },
                        "lineno": {
                            "type": "integer",
                            "description": "The line number where the symbol is present."
                        },
                        "symbol": {
                            "type": "string",
                            "description": "The symbol to lookup."
                        }
                    },
                    "required": [ "filename", "lineno", "symbol" ]
                }
            }
            """
            return _capture_onecmd(debugger, f"definition {filename}:{lineno} {symbol}")

        assistant.add_function(llm_find_definition)

    return assistant


def get_frame_summary(
    debugger: lldb.SBDebugger, max_entries: int = 20
) -> Optional[str]:
    thread = get_thread(debugger)
    if not thread:
        return None

    class FrameSummaryEntry:
        def __init__(self, text: str):
            self._text = text

        def __str__(self):
            return self._text

        def __repr__(self):
            return f"FrameSummaryEntry({repr(self._text)})"

    class SkippedFramesEntry:
        def __init__(self, count: int):
            self._count = count

        def count(self):
            return self._count

        def __str__(self):
            return f"[{self._count} skipped frames...]"

        def __repr__(self):
            return f"SkippedFramesEntry({self._count})"

    total_frames = len(thread)  # This can be a long operation e.g. stack overflow.
    skipped = 0
    summaries: List[Union[FrameSummaryEntry, SkippedFramesEntry]] = []

    index = -1
    for frame in thread:
        index += 1
        if not frame.GetDisplayFunctionName():
            skipped += 1
            continue
        name = frame.GetDisplayFunctionName().split("(")[0]
        arguments = []
        for j in range(
            frame.GetFunction().GetType().GetFunctionArgumentTypes().GetSize()
        ):
            arg = frame.FindVariable(frame.GetFunction().GetArgumentName(j))
            if not arg:
                skipped += 1
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
            skipped += 1
            continue

        if skipped > 0:
            summaries.append(SkippedFramesEntry(skipped))
            skipped = 0

        summaries.append(
            FrameSummaryEntry(
                f"{index}: {name}({', '.join(arguments)}) at {file_path}:{lineno}"
            )
        )
        if len(summaries) >= max_entries:
            break

    if skipped > 0:
        summaries.append(SkippedFramesEntry(skipped))
        if len(summaries) > max_entries:
            summaries.pop(-2)

    total_summary_count = sum(
        [1 if isinstance(s, FrameSummaryEntry) else s.count() for s in summaries]
    )

    if total_summary_count < total_frames:
        if isinstance(summaries[-1], SkippedFramesEntry):
            summaries[-1] = SkippedFramesEntry(
                total_frames - total_summary_count + summaries[-1].count()
            )
        else:
            summaries.append(SkippedFramesEntry(total_frames - total_summary_count + 1))
            if len(summaries) > max_entries:
                summaries.pop(-2)

    assert (
        sum([1 if isinstance(s, FrameSummaryEntry) else s.count() for s in summaries])
        == total_frames
    )

    return "\n".join([str(s) for s in summaries])


def get_error_message(debugger: lldb.SBDebugger) -> Optional[str]:
    thread = get_thread(debugger)
    return thread.GetStopDescription(1024) if thread else None


_assistant = None


@lldb.command("chat")
def chat(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
):
    if not debugger.GetSelectedTarget():
        result.SetError("must be attached to a program to use `chat`.")
        return
    if not is_debug_build(debugger):
        result.SetError(
            "your program must be compiled with debug information (`-g`) to use `chat`."
        )
        return
    thread = get_thread(debugger)
    if not thread:
        result.SetError("must run the code first to use `chat`.")
        return

    args, remaining = chatdbg_utils.parse_known_args(command.split())

    global _assistant
    if not _assistant or args.fresh:
        _assistant = _make_assistant(debugger, args, result)

    parts = []

    if _assistant.conversation_size() == 1:
        error_message = get_error_message(debugger)
        if not error_message:
            result.AppendWarning("could not generate an error message.")
        else:
            parts.append(
                "Here is the reason the program stopped execution:\n```\n"
                + error_message
                + "\n```"
            )

        frame_summary = get_frame_summary(debugger)
        if not frame_summary:
            result.AppendWarning("could not generate a frame summary.")
        else:
            parts.append(
                "Here is a summary of the stack frames, omitting those not associated with user source code:\n```\n"
                + frame_summary
                + "\n```"
            )

    parts.append(" ".join(remaining) if remaining else "What's the problem?")

    prompt = "\n\n".join(parts)

    _assistant.run(
        prompt,
        result.AppendMessage,
        result.AppendWarning,
        result.SetError,
    )


@lldb.command("repl")
def repl(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
):
    if command.strip():
        result.AppendWarning("`repl` does not take any arguments (arguments ignored).")

    while True:
        try:
            command = input(f"{PROMPT} ").strip()
        except EOFError:
            break

        if command == "exit":
            break
        result = _capture_onecmd(debugger, command)
        print("-----------------------------------")
        print(result, end="")
        print("-----------------------------------")
