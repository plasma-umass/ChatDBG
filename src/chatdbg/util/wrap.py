import textwrap


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
    for i in range(0, len(blocks), 2):
        paras = blocks[i].split("\n")
        wrapped = [textwrap.fill(para, width=width) for para in paras]
        blocks[i] = "\n".join(wrapped)

    return "```".join(blocks)
