from typing import Optional

import llm_utils


class BaseFunctions:
    def __init__(self, args):
        self.args = args

    def as_tools(self):
        return [
            {"type": "function", "function": schema}
            for schema in [self.get_code_surrounding_schema()]
        ]

    def dispatch(self, name, arguments) -> Optional[str]:
        if name == "get_code_surrounding":
            filename = arguments["filename"]
            lineno = arguments["lineno"]
            result = self.get_code_surrounding(filename, lineno)

            print(f"Calling get_code_surrounding({filename}, {lineno})...")
            print(result)
            print()

            return result
        return None

    def get_code_surrounding_schema(self):
        return {
            "name": "get_code_surrounding",
            "description": "Returns the code in the given file surrounding and including the provided line number.",
            "parameters": {
                "type": "object",
                "properties": {
                    "filename": {
                        "type": "string",
                        "description": "The filename to read from.",
                    },
                    "lineno": {
                        "type": "integer",
                        "description": "The line number to focus on. Some context before and after that line will be provided.",
                    },
                },
                "required": ["filename", "lineno"],
            },
        }

    def get_code_surrounding(self, filename: str, lineno: int) -> str:
        (lines, first) = llm_utils.read_lines(filename, lineno - 7, lineno + 3)
        return llm_utils.number_group_of_lines(lines, first)
