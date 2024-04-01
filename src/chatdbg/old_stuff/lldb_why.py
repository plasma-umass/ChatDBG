
# def truncate_string(string, n):
#     if len(string) <= n:
#         return string
#     else:
#         return string[:n] + "..."


# def buildPrompt(debugger: Any) -> Tuple[str, str, str]:
#     target = debugger.GetSelectedTarget()
#     if not target:
#         return ("", "", "")
#     thread = get_thread(debugger)
#     if not thread:
#         return ("", "", "")
#     if thread.GetStopReason() == lldb.eStopReasonBreakpoint:
#         return ("", "", "")
#     frame = thread.GetFrameAtIndex(0)
#     stack_trace = ""
#     source_code = ""

#     # magic number - don't bother walking up more than this many frames.
#     # This is just to prevent overwhelming OpenAI (or to cope with a stack overflow!).
#     max_frames = 10

#     index = 0
#     for frame in thread:
#         if index >= max_frames:
#             break
#         function = frame.GetFunction()
#         if not function:
#             continue
#         full_func_name = frame.GetFunctionName()
#         func_name = full_func_name.split("(")[0]
#         arg_list = []

#         # Build up an array of argument values to the function, with type info.
#         for i in range(len(frame.GetFunction().GetType().GetFunctionArgumentTypes())):
#             arg = frame.FindVariable(frame.GetFunction().GetArgumentName(i))
#             if not arg:
#                 continue
#             arg_name = str(arg).split("=")[0].strip()
#             arg_val = str(arg).split("=")[1].strip()
#             arg_list.append(f"{arg_name} = {arg_val}")

#         # Get the frame variables
#         variables = frame.GetVariables(True, True, True, True)
#         var_list = []

#         for var in variables:
#             name = var.GetName()
#             value = var.GetValue()
#             type = var.GetTypeName()
#             # Check if the value is a pointer
#             if var.GetType().IsPointerType():
#                 # Attempt to dereference the pointer
#                 try:
#                     deref_value = var.Dereference().GetValue()
#                     var_list.append(
#                         f"{type} {name} = {value} (*{name} = {deref_value})"
#                     )
#                 except:
#                     var_list.append(f"{type} {name} = {value}")

#         line_entry = frame.GetLineEntry()
#         file_path = line_entry.GetFileSpec().fullpath
#         lineno = line_entry.GetLine()
#         col_num = line_entry.GetColumn()

#         # If we are in a subdirectory, use a relative path instead.
#         if file_path.startswith(os.getcwd()):
#             file_path = os.path.relpath(file_path)

#         max_line_length = 100

#         try:
#             lines, first = llm_utils.read_lines(file_path, lineno - 10, lineno)
#             block = llm_utils.number_group_of_lines(lines, first)

#             stack_trace += (
#                 truncate_string(
#                     f'frame {index}: {func_name}({",".join(arg_list)}) at {file_path}:{lineno}:{col_num}',
#                     max_line_length - 3,  # 3 accounts for ellipsis
#                 )
#                 + "\n"
#             )
#             if len(var_list) > 0:
#                 for var in var_list:
#                     stack_trace += "  " + truncate_string(var, max_line_length) + "\n"
#             source_code += f"/* frame {index} in {file_path} */\n"
#             source_code += block + "\n\n"
#         except:
#             # Couldn't find the source for some reason. Skip the file.
#             continue
#         index += 1
#     error_reason = thread.GetStopDescription(255)
#     # If the Rust panic log exists, append it to the error reason.
#     try:
#         with open(RUST_PANIC_LOG_FILENAME, "r") as log:
#             panic_log = log.read()
#         error_reason = panic_log + "\n" + error_reason
#     except:
#         pass
#     return (source_code.strip(), stack_trace.strip(), error_reason.strip())


# @lldb.command("why")
# def why(
#     debugger: lldb.SBDebugger,
#     command: str,
#     result: lldb.SBCommandReturnObject,
#     internal_dict: dict,
# ) -> None:
#     """
#     The why command is where we use the refined stack trace system.
#     We send information once to GPT, and receive an explanation.
#     There is a bit of work to determine what context we end up sending to GPT.
#     Notably, we send a summary of all stack frames, including locals.
#     """
#     if not debugger.GetSelectedTarget():
#         result.SetError("must be attached to a program to ask `why`.")
#         return
#     if not is_debug_build(debugger):
#         result.SetError(
#             "your program must be compiled with debug information (`-g`) to use `why`."
#         )
#         return
#     thread = get_thread(debugger)
#     if not thread:
#         result.SetError("must run the code first to ask `why`.")
#         return

#     the_prompt = buildPrompt(debugger)
#     args, _ = chatdbg_utils.parse_known_args(command.split())
#     chatdbg_utils.explain(
#         the_prompt[0],
#         the_prompt[1],
#         the_prompt[2],
#         args,
#         result.AppendMessage,
#         result.AppendWarning,
#         result.SetError,
#     )