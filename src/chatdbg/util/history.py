class CommandHistory:
    def __init__(self, prompt: str):
        self._history: list[tuple[str, str]] = []
        self._prompt = prompt

    def append(self, command: str, result: str) -> None:
        self._history += [(command, result)]

    def clear(self) -> None:
        self._history = []

    def _format_history_entry(self, entry: tuple[str, str]) -> str:
        line, output = entry
        if output:
            return f"{self._prompt}{line}\n{output}"
        else:
            return f"{self._prompt}{line}"

    def __str__(self) -> str:
        entry_strs = [self._format_history_entry(x) for x in self._history]
        return "\n".join(entry_strs)
