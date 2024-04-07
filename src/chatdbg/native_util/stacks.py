
from typing import List


class _ArgumentEntry:
    def __init__(self, type: str, name: str, value: str):
        self._type = type
        self._name = name
        self._value = value

    def __str__(self):
        return f"({self._type}) {self._name} = {self._value if self._value else '[unknown]'}"

    def __repr__(self):
        return f"_ArgumentEntry({repr(self.type)}, {repr(self._name)}, {repr(self._value)})"


class _FrameSummaryEntry:
    def __init__(
        self,
        index: int,
        name: str,
        arguments: List[_ArgumentEntry],
        file_path: str,
        lineno: int,
    ):
        self._index = index
        self._name = name
        self._arguments = arguments
        self._file_path = file_path
        self._lineno = lineno

    def index(self):
        return self._index

    def file_path(self):
        return self._file_path

    def lineno(self):
        return self._lineno

    def __str__(self):
        return f"{self._index}: {self._name}({', '.join([str(a) for a in self._arguments])}) at {self._file_path}:{self._lineno}"

    def __repr__(self):
        return f"_FrameSummaryEntry({self._index}, {repr(self._name)}, {repr(self._arguments)}, {repr(self._file_path)}, {self._lineno})"


class _SkippedFramesEntry:
    def __init__(self, count: int):
        self._count = count

    def count(self):
        return self._count

    def __str__(self):
        return f"[{self._count} skipped frame{'s' if self._count > 1 else ''}...]"

    def __repr__(self):
        return f"_SkippedFramesEntry({self._count})"

