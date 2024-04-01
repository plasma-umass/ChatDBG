import textwrap

PROMPT = "(ChatDBG lldb) "


def _format_history_entry(entry, indent=""):
        line, output = entry
        if output:
            entry = f"{PROMPT}{line}\n{output}"
        else:
            entry = f"{PROMPT}{line}"
        return textwrap.indent(entry, indent, lambda _: True)


class History:
    def __init__(self):
        self._history = []


    def clear_history(self):
        self._history = []


    def make_entry(self, command, result):
        self._history += [(command, result)]


    def do_history(self):
        """
        Returns the formatted history of user-issued commands since the last chat.
        """
        entry_strs = [_format_history_entry(x) for x in self._history]
        history_str = "\n".join(entry_strs)
        return history_str


    def get_history(self):
            if len(self._history) > 0:
                hist = textwrap.indent(self.do_history(), "")
                hist = f"\nThis is the history of some lldb commands I ran and the results.\n```\n{hist}\n```\n"
                return hist
            else:
                return ""
            