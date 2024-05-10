import os
import tomllib
import re

DIRNAME = os.path.dirname(__file__)
PYPROJECT_FILE = os.path.join(DIRNAME, "../pyproject.toml")
CARGO_TOML_FILES = [
    os.path.join(DIRNAME, "chatdbg/Cargo.toml"),
    os.path.join(DIRNAME, "chatdbg_macros/Cargo.toml"),
]


if __name__ == "__main__":
    with open(PYPROJECT_FILE, "rb") as f:
        version = tomllib.load(f)["project"]["version"]
    for file in CARGO_TOML_FILES:
        with open(file, "r") as f:
            content = f.read()
        content = re.sub(r"##VERSION##", version, content)
        with open(file, "w") as f:
            f.write(content)
