from io import StringIO, TextIOWrapper


class CaptureInput:
    def __init__(self, input_stream):
        input_stream = TextIOWrapper(input_stream.buffer, encoding="utf-8", newline="")

        self.original_input = input_stream
        self.capture_buffer = StringIO()
        self.original_readline = input_stream.buffer.raw.readline

        def custom_readline(*args, **kwargs):
            input_data = self.original_readline(*args, **kwargs)
            self.capture_buffer.write(input_data.decode())
            return input_data

        input_stream.buffer.raw.readline = custom_readline

    def readline(self, *args, **kwargs):
        input_data = self.original_input.readline(*args, **kwargs)
        self.capture_buffer.write(input_data)
        self.capture_buffer.flush()
        return input_data

    def read(self, *args, **kwargs):
        input_data = self.original_input.read(*args, **kwargs)
        self.capture_buffer.write(input_data)
        self.capture_buffer.flush()
        return input_data

    def get_captured_input(self):
        return self.capture_buffer.getvalue()


class CaptureOutput:
    """
    File wrapper that will stash a copy of everything written.
    """

    def __init__(self, file):
        self.file = file
        self.buffer = StringIO()

    def write(self, data):
        self.buffer.write(data)
        return self.file.write(data)

    def getvalue(self):
        return self.buffer.getvalue()

    def getfile(self):
        return self.file

    def __getattr__(self, attr):
        # Delegate attribute access to the file object
        return getattr(self.file, attr)
