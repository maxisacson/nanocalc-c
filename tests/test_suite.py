# import pytest
import subprocess


def evaluate(txt):
    res = subprocess.run("../nc", input=txt, text=True, capture_output=True)
    return res.stdout.splitlines()


def test_init():
    lines = evaluate("1+2")
    assert lines[-1] == "3"
