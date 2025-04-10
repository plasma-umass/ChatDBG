import os
import pathlib
import random

ROOT = os.path.dirname(os.path.abspath(__file__))
TOTAL_LENGTH = 1024
CHUNK_LENGTH = 128


def random_name(size: int) -> str:
    return "".join(
        random.choice("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
        for _ in range(size)
    )


remaining = TOTAL_LENGTH
path = ROOT
while remaining > CHUNK_LENGTH:
    name = random_name(CHUNK_LENGTH)
    remaining -= CHUNK_LENGTH
    path = os.path.join(path, name)
    os.mkdir(path)
name = random_name(remaining)
pathlib.Path(os.path.join(path, name)).touch()
