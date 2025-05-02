from typing import Any

_chatdbg_was_called = False


def chatdbg_was_called() -> None:
    global _chatdbg_was_called
    _chatdbg_was_called = True


def print_exit_message(*args: Any, **kwargs: Any) -> None:
    global _chatdbg_was_called
    if _chatdbg_was_called:
        print("Thank you for using ChatDBG!")
        print(
            "Share your success stories here: https://github.com/plasma-umass/ChatDBG/issues/53"
        )
