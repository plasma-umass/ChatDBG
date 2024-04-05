class CommandHistory:

    def __init__(self, prompt):
        self._history = [ ]
        self._prompt = prompt

    def append(self, command, result):
        self._history += [ (command, result) ]

    def clear(self):
        self._history = [ ]

    def _format_history_entry(self, entry):
        line, output = entry
        if output:
            return f"{self._prompt}{line}\n{output}"
        else:
            return f"{self._prompt}{line}"

    def __str__(self):
        entry_strs = [self._format_history_entry(x) for x in self._history]
        return "\n".join(entry_strs)
