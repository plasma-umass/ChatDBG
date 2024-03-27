from .assistant import Assistant
from .listeners import StreamingPrinter, Printer

if __name__ == "__main__":

    def weather(location, unit="f"):
        """
        {
            "name": "get_weather",
            "description": "Determine weather in my location",
            "parameters": {
                "type": "object",
                "properties": {
                "location": {
                    "type": "string",
                    "description": "The city and state e.g. San Francisco, CA"
                },
                "unit": {
                    "type": "string",
                    "enum": [
                    "c",
                    "f"
                    ]
                }
                },
                "required": [
                "location"
                ]
            }
        }
        """
        return f"weather({location}, {unit})", "Sunny and 72 degrees."

    a = Assistant(
        "You generate text.", clients=[StreamingPrinter()], functions=[weather]
    )
    x = a.query(
        "tell me what model you are before making any function calls.  And what's the weather in Boston?",
        stream=True,
    )
    print(x)
