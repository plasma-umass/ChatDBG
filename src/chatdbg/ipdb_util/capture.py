from io import StringIO, TextIOWrapper

class CaptureInput:
    def __init__(self, input_stream):
        input_stream = TextIOWrapper(input_stream.buffer, encoding='utf-8', newline='')

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
