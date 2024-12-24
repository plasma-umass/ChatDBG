import tomllib
import sys

from packaging.version import Version
import requests

PYPI_URL = "https://pypi.org/pypi/chatdbg/json"

latest = requests.get(PYPI_URL).json()["info"]["version"]
with open("pyproject.toml", "rb") as f:
    current = tomllib.load(f)["project"]["version"]

if Version(current) <= Version(latest):
    print(f"Latest version is {latest} on PyPI. Please bump versions.")
    sys.exit(1)
