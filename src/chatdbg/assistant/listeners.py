import sys
import textwrap


class BaseAssistantListener:
    """
    Events that the Assistant generates.  Override these for the client.
    """

    # Dialogs capture 1 or more queries.

    def on_begin_dialog(self, instructions):
        pass

    def on_end_dialog(self):
        pass

    # Events for a single query

    def on_begin_query(self, prompt, user_text):
        pass

    def on_response(self, text):
        pass

    def on_function_call(self, call, result):
        pass

    def on_end_query(self, stats):
        pass

    # For clients wishing to stream responses

    def on_begin_stream(self):
        pass

    def on_stream_delta(self, text):
        pass

    def on_end_stream(self):
        pass

    # Notifications of non-fatal problems

    def on_warn(self, text):
        pass

    def on_error(self, text):
        pass


class Printer(BaseAssistantListener):
    def __init__(self, out=sys.stdout):
        self.out = out

    def on_warn(self, text):
        print(textwrap.indent(text, "*** "), file=self.out)

    def on_error(self, text):
        print(textwrap.indent(text, "*** "), file=self.out)

    def on_begin_stream(self):
        pass

    def on_stream_delta(self, text):
        print(text, end="", file=self.out, flush=True)

    def on_end_stream(self):
        pass

    def on_begin_query(self, prompt, user_text):
        pass

    def on_end_query(self, stats):
        pass

    def on_response(self, text):
        if text != None:
            print(text, file=self.out)

    def on_function_call(self, call, result):
        if result and len(result) > 0:
            entry = f"{call}\n{result}"
        else:
            entry = f"{call}"
        print(entry, file=self.out)


class StreamingPrinter(Printer):
    def __init__(self, out=sys.stdout):
        super().__init__(out)

    def on_begin_stream(self):
        print("", flush=True)

    def on_stream_delta(self, text):
        print(text, end="", file=self.out, flush=True)

    def on_end_stream(self):
        print("", flush=True)

    def on_response(self, text):
        pass
