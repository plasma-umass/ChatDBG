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
            summaries.append(f"{i}: {name}({', '.join(arguments)})")
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
