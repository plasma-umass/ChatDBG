
import sys
import textwrap


class AbsAssistantListener:

    def begin_dialog(self, instructions):
        pass

    def end_dialog(self):
        pass

    def begin_query(self, prompt, extra):
        pass

    def end_query(self, stats):
        pass

    def warn(self, text):
        pass

    def fail(self, text):
        pass

    def begin_stream(self):
        pass

    def stream_delta(self, text):
        pass

    def end_stream(self):
        pass

    def response(self, text):
        pass

    def function_call(self, call, result):
        pass


class Printer(AbsAssistantListener):
    def __init__(self, out=sys.stdout):
        self.out = out

    def warn(self, text):
        print(textwrap.indent(text, "*** "), file=self.out)

    def fail(self, text):
        print(textwrap.indent(text, "*** "), file=self.out)
        sys.exit(1)

    def begin_stream(self):
        pass

    def stream_delta(self, text):
        print(text, end="", file=self.out, flush=True)

    def end_stream(self):
        pass

    def begin_query(self, prompt, extra):
        pass

    def end_query(self, stats):
        pass

    def response(self, text):
        if text != None:
            print(text, file=self.out)

    def function_call(self, call, result):
        if result and len(result) > 0:
            entry = f"{call}\n{result}"
        else:
            entry = f"{call}"
        print(entry, file=self.out)


class StreamingPrinter(Printer):
    def __init__(self, out=sys.stdout):
        super().__init__(out)

    def begin_stream(self):
        print("", flush=True)

    def stream_delta(self, text):
        print(text, end="", file=self.out, flush=True)

    def end_stream(self):
        print("", flush=True)

    def response(self, text):
        pass
