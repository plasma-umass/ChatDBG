import os
from typing import Optional

import lldb

from .functions_interface import BaseFunctions


class LldbFunctions(BaseFunctions):
    def __init__(self, args):
        super().__init__(args)

    def as_tools(self):
        return super().as_tools() + [
            {"type": "function", "function": schema} for schema in []
        ]

    def dispatch(self, name, arguments) -> Optional[str]:
        return super().dispatch(name, arguments)

    def get_frame_summary(self) -> str:
        target = lldb.debugger.GetSelectedTarget()
        if not target:
            return None

        for thread in target.process:
            reason = thread.GetStopReason()
            if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
                break

        summaries = []
        index = 0
        for frame in thread:
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

            summaries.append(
                f"{index}: {name}({', '.join(arguments)}) at {file_path}:{lineno}"
            )
            index += 1
        return "\n".join(summaries)

    def get_error_message(self) -> Optional[str]:
        target = lldb.debugger.GetSelectedTarget()
        if not target:
            return None

        for thread in target.process:
            reason = thread.GetStopReason()
            if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
                break

        return thread.GetStopDescription(1024)
