from typing import Optional

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
