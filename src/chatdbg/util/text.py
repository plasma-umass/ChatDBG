import re
import itertools
import inspect
import numbers
import numpy as np
import textwrap


def make_arrow(pad):
    """generate the leading arrow in front of traceback or debugger"""
    if pad >= 2:
        return "-" * (pad - 2) + "> "
    elif pad == 1:
        return ">"
    return ""


def strip_color(s):
    ansi_escape = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")
    return ansi_escape.sub("", s)


def _is_iterable(obj):
    try:
        iter(obj)
        return True
    except TypeError:
        return False


def _repr_if_defined(obj):
    if obj.__class__ in [np.ndarray, dict, list, tuple]:
        # handle these at iterables to truncate reasonably
        return False
    result = (
        "__repr__" in dir(obj.__class__)
        and obj.__class__.__repr__ is not object.__repr__
    )
    return result


def format_limited(value, limit=10, depth=3):
    def format_tuple(t, depth):
        return tuple([helper(x, depth) for x in t])

    def format_list(list, depth):
        return [helper(x, depth) for x in list]

    def format_dict(items, depth):
        return {k: helper(v, depth) for k, v in items}

    def format_object(obj, depth):
        attributes = dir(obj)
        fields = {
            attr: getattr(obj, attr, None)
            for attr in attributes
            if not callable(getattr(obj, attr, None)) and not attr.startswith("__")
        }
        return format(
            f"{type(obj).__name__} object with fields {format_dict(fields.items(), depth)}"
        )

    def helper(value, depth):
        if depth == 0:
            return ...
        if value is Ellipsis:
            return ...
        if isinstance(value, dict):
            if len(value) > limit:
                return format_dict(
                    list(value.items())[: limit - 1] + [(..., ...)], depth - 1
                )
            else:
                return format_dict(value.items(), depth - 1)
        elif isinstance(value, (str, bytes)):
            if len(value) > 254:
                value = str(value)[0:253] + "..."
            return value
        elif isinstance(value, tuple):
            if len(value) > limit:
                return format_tuple(value[0 : limit - 1] + (...,), depth - 1)
            else:
                return format_tuple(value, depth - 1)
        elif value is None or isinstance(
            value, (int, float, bool, type, numbers.Number)
        ):
            return value
        elif isinstance(value, np.ndarray):
            with np.printoptions(threshold=limit):
                return np.array_repr(value)
        elif inspect.isclass(type(value)) and _repr_if_defined(value):
            return repr(value)
        elif _is_iterable(value):
            value = list(itertools.islice(value, 0, limit + 1))
            if len(value) > limit:
                return format_list(value[: limit - 1] + [...], depth - 1)
            else:
                return format_list(value, depth - 1)
        elif inspect.isclass(type(value)):
            return format_object(value, depth - 1)
        else:
            return value

    result = str(helper(value, depth=3)).replace("Ellipsis", "...")
    if len(result) > 1024 * 2:
        result = result[: 1024 * 2 - 3] + "..."
    if type(value) == str:
        return "'" + result + "'"
    else:
        return result


def truncate_proportionally(text, maxlen=32000, top_proportion=0.5):
    """Omit part of a string if needed to make it fit in a maximum length."""
    if len(text) > maxlen:
        pre = max(0, int((maxlen - 3) * top_proportion))
        post = max(0, maxlen - 3 - pre)
        return text[:pre] + "..." + text[len(text) - post :]
    return text


def word_wrap_except_code_blocks(text: str, width: int = 80) -> str:
    """
    Wraps text except for code blocks for nice terminal formatting.

    Splits the text into paragraphs and wraps each paragraph,
    except for paragraphs that are inside of code blocks denoted
    by ` ``` `. Returns the updated text.

    Args:
        text (str): The text to wrap.
        width (int): The width of the lines to wrap at, passed to `textwrap.fill`.

    Returns:
        The wrapped text.
    """
    blocks = text.split("```")
    for i in range(len(blocks)):
        if i % 2 == 0:
            paras = blocks[i].split("\n")
            wrapped = [textwrap.fill(para, width=width) for para in paras]
            blocks[i] = "\n".join(wrapped)

    return "```".join(blocks)
