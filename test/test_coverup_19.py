# file src/chatdbg/pdb/prompts.py:76-83
# lines [76, 77, 78, 79, 81, 83]
# branches ['77->78', '77->83', '78->79', '78->81']

import pytest

# Assuming the existence of the following constants in chatdbg/pdb/prompts.py
# _wheel_and_slice = "You are a debugging assistant. You will be given a Python stack trace for an error and answer questions related to the root cause of the error.\n\nCall the `pdb` function to run Pdb debugger commands on the stopped program. You may call the `pdb` function to run the following commands: `bt`, `up`, `down`, ..."
# _wheel_no_slice = "Instructions for wheel without slice."
# _no_wheel = "Instructions without wheel."

from chatdbg.custom_pdb.prompts import pdb_instructions

def test_pdb_instructions_wheel_and_slice(monkeypatch):
    expected_output = "You are a debugging assistant. You will be given a Python stack trace for an error and answer questions related to the root cause of the error.\n\nCall the `pdb` function to run Pdb debugger commands on the stopped program. You may call the `pdb` function to run the following commands: `bt`, `up`, `down`, ..."
    monkeypatch.setattr('chatdbg.custom_pdb.prompts._wheel_and_slice', expected_output)
    assert pdb_instructions(supports_flow=True, take_the_wheel=True) == expected_output

def test_pdb_instructions_wheel_no_slice(monkeypatch):
    expected_output = "Instructions for wheel without slice."
    monkeypatch.setattr('chatdbg.custom_pdb.prompts._wheel_no_slice', expected_output)
    assert pdb_instructions(supports_flow=False, take_the_wheel=True) == expected_output

def test_pdb_instructions_no_wheel(monkeypatch):
    expected_output = "Instructions without wheel."
    monkeypatch.setattr('chatdbg.custom_pdb.prompts._no_wheel', expected_output)
    assert pdb_instructions(supports_flow=False, take_the_wheel=False) == expected_output
    assert pdb_instructions(supports_flow=True, take_the_wheel=False) == expected_output
