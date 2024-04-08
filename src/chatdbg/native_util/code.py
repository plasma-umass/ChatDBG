import llm_utils


def code(command):
    parts = command.split(":")
    if len(parts) != 2:
        return "usage: code <filename>:<lineno>"
    filename, lineno = parts[0], int(parts[1])
    try:
        lines, first = llm_utils.read_lines(filename, lineno - 7, lineno + 3)
    except FileNotFoundError:
        return f"file '{filename}' not found."
    return llm_utils.number_group_of_lines(lines, first)
