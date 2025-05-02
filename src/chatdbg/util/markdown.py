import shutil
import textwrap

from rich import box
import rich
from rich.console import Console
from rich.live import Live
from rich.markdown import Markdown
from rich.panel import Panel
from rich.theme import Theme
from rich.table import Table
from rich.markup import escape

from chatdbg.util.text import fill_to_width, wrap_long_lines

from ..assistant.listeners import BaseAssistantListener


_themes = {
    "default": Theme(
        {
            "markdown.block": "black on light_steel_blue1",
            "markdown.paragraph": "black on light_steel_blue1",
            "markdown.text": "black on light_steel_blue1",
            "markdown.code": "blue",
            "markdown.code_block": "blue",
            "markdown.item.bullet": "bold blue",
            "markdown.item.number": "bold blue",
            "markdown.h1": "bold black",
            "markdown.h2": "bold black",
            "markdown.h3": "bold black",
            "markdown.h4": "bold black",
            "markdown.h5": "bold black",
            "command": "bold gray11 on wheat1",
            "result": "grey35 on wheat1",
            "warning": "bright_white on green",
            "error": "bright_white on red",
        }
    ),
    "basic": Theme(
        {
            "markdown.block": "bright_blue on bright_white",
            "markdown.paragraph": "bright_blue on bright_white",
            "markdown.text": "bright_blue on bright_white",
            "markdown.code": "cyan",
            "markdown.code_block": "bright_blue",
            "markdown.item.bullet": "bold cyan",
            "markdown.item.number": "bold cyan",
            "markdown.h1": "bold black",
            "markdown.h2": "bold black",
            "markdown.h3": "bold black",
            "markdown.h4": "bold black",
            "markdown.h5": "bold bright_blue on bright_white",
            "command": "bold bright_yellow on white",
            "result": "yellow on white",
            "warning": "bright_white on green",
            "error": "bright_white on red",
        }
    ),
}

_simple_box = Box = box.Box(
    "    \n" "    \n" "    \n" "    \n" "    \n" "    \n" "    \n" "    \n", ascii=True
)

from rich.markdown import ConsoleOptions, loop_first, RenderResult, Segment


class MyListItem(rich.markdown.ListItem):
    """An item in a list."""

    style_name = "markdown.item"

    def __init__(self) -> None:
        super().__init__()

    def render_bullet(self, console: Console, options: ConsoleOptions) -> RenderResult:
        render_options = options.update(width=options.max_width - 3)
        lines = console.render_lines(self.elements, render_options, style=self.style)
        bullet_style = console.get_style("markdown.item.bullet", default="none")

        bullet = Segment(" * ", bullet_style)
        padding = Segment(" " * 3, bullet_style)
        new_line = Segment("\n")
        for first, line in loop_first(lines):
            yield bullet if first else padding
            yield from line
            yield new_line


class ChatDBGMarkdownPrinter(BaseAssistantListener):

    def __init__(self, out, debugger_prompt, chat_prefix, width, theme=None):
        self._out = out
        self._debugger_prompt = debugger_prompt
        self._chat_prefix = chat_prefix
        self._left_indent = 4
        self._width = shutil.get_terminal_size(fallback=(width, 24)).columns
        self._theme = _themes["default"] if theme == None else _themes[theme]
        self._code_theme = "default"
        # used to keep track of streaming
        self._streamed = ""

        self._console = self._make_console(out)

        if theme == "basic":
            Markdown.elements["list_item_open"] = MyListItem
            self._code_theme = "monokai"

    def _make_console(self, out):
        return Console(soft_wrap=False, file=out, theme=self._theme, width=self._width)

    # Call backs

    def on_begin_query(self, prompt, user_text):
        pass

    def on_end_query(self, stats):
        pass

    def _print(self, renderable, end=""):
        self._console.print(renderable, end=end)

    def _wrap_in_panel(self, rich_element):

        left_panel = Panel("", box=_simple_box, style="on default")
        right_panel = Panel(
            rich_element,
            box=_simple_box,
            style=self._console.get_style("markdown.block"),
        )

        # Create a table to hold the panels side by side
        table = Table.grid(padding=0)
        table.add_column(justify="left", width=self._left_indent - 2)
        table.add_column(justify="left")
        table.add_row(left_panel, right_panel)
        return table

    def _message(self, text, style):
        self._print(
            self._wrap_in_panel(self._wrap_and_fill_and_indent(text, " *** ", style))
        )

    def on_warn(self, text):
        self._message(text, "warning")

    def on_error(self, text):
        self._message(text, "error")

    def _stream_append(self, text):
        self._streamed += text
        m = self._wrap_in_panel(Markdown(self._streamed, code_theme=self._code_theme))
        self._live.update(m)

    def on_begin_stream(self):
        self._streamed = ""

    def on_stream_delta(self, text):
        if self._streamed == "":
            self._live = Live(vertical_overflow="visible", console=self._console)
            self._live.start(True)
        self._stream_append(text)

    def on_end_stream(self):
        if self._streamed != "":
            self._live.stop()

    def on_response(self, text):
        if self._streamed == "" and text != None:
            m = self._wrap_in_panel(Markdown(text, code_theme=self._code_theme))
            self._print(m, end="\n")
        self._streamed = ""

    def on_function_call(self, call, result):
        prefix = self._chat_prefix
        line = fill_to_width(f"\n{prefix}{self._debugger_prompt}{call}", self._width)
        entry = f"[command]{escape(line)}[/]\n"

        entry += self._wrap_and_fill_and_indent(
            result.rstrip() + "\n", prefix, "result"
        )
        m = self._wrap_in_panel(entry)
        self._print(m, end="")

    def _wrap_and_fill_and_indent(self, text, prefix, style_name):
        line_width = self._width - len(prefix) - self._left_indent - 2
        text = wrap_long_lines(text.expandtabs(), line_width, subsequent_indent="    ")
        text = fill_to_width(text, line_width)
        text = textwrap.indent(text, prefix, lambda _: True)
        text = escape(text)
        return f"[{style_name}]{text}[/]"
