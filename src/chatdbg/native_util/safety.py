import re


# A very simple whitelist-based approach.
# If ChatDBG wants to call other commands not listed here, they should be
# evaluated and added if not possibly harmful.
def command_is_safe(cmd: str) -> bool:
    cmd = cmd.strip()
    command_name = cmd.split()[0]

    # Allowed unconditionally.
    if command_name in [
        "apropos",
        "bt",
        "down",
        "frame",
        "h",
        "help",
        "info",
        "language",
        "l",
        "list",
        "source",
        "up",
        "version",
    ]:
        return True

    # Allowed conditionally.
    if command_name in ["p", "print"]:
        return re.fullmatch(r"([a-zA-Z0-9_ *.]|->)*", cmd) is not None

    return False
