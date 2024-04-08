from io import StringIO

from IPython.display import HTML, display, update_display
from rich.console import Console
from rich.markdown import Markdown
from chatdbg.util.markdown import ChatDBGMarkdownPrinter


class ChatDBGJupyterPrinter(ChatDBGMarkdownPrinter):

    def __init__(self, debugger_prompt, chat_prefix, width):
        super().__init__(StringIO(), debugger_prompt, chat_prefix, width)

    def _make_console(self, out):
        return Console(
            soft_wrap=False, file=out, record=True, theme=self._theme, width=self._width
        )

    # Call backs

    # override to flush to the display
    def _print(self, text, end=""):
        super()._print(text, end=end)
        display(HTML(self._export_html()))

    def _export_html(self):
        exported_html = self._console.export_html(clear=True, inline_styles=True)
        custom_css = """
        <style>
            .rich-text pre,code,div {
                line-height: normal !important;
                margin-bottom: 0 !important;
                font-size: 14px !important;
            }

            .rich-text .jp-RenderedHTMLCommon pre, .jp-RenderedHTMLCommon code {
                white-space: pre;
            }            
        </style>
        """
        exported_html = f'<div class="rich-text">{exported_html}</div>'
        modified_html = custom_css + exported_html
        return modified_html

    def _stream_append(self, text):
        self._streamed += text
        m = self._wrap_in_panel(Markdown(self._streamed, code_theme=self._code_theme))
        self._console.print(m)
        exported_html = self._export_html()
        update_display(HTML(exported_html), display_id=self._display_handle.display_id)

    def on_begin_stream(self):
        self._streamed = ""

    def on_stream_delta(self, text):
        if self._streamed == "":
            self._display_handle = display(HTML(""), display_id=True)
        self._stream_append(text)

    def on_end_stream(self):
        pass
