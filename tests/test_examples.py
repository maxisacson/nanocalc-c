import pytest
import subprocess
import sys
import os


def run_example(filename):
    path = os.path.join(f"../examples/{filename}")
    with open(path) as f:
        res = subprocess.run("../nc", input=f.read(), text=True, capture_output=True)

    if res.stderr:
        print(res.stderr, file=sys.stderr)

    return res.stdout


def test_block():
    r = run_example("block.nc")
    expect = """\
f(0) 1
f(1) 2
"""
    assert r == expect


def test_block2():
    r = run_example("block2.nc")
    assert r == "11 22\n"


def test_cases():
    r = run_example("cases.nc")
    assert r == "positive\n"


def test_cases2():
    r = run_example("cases2.nc")
    expect = """\
f(0) 0
f(1) 1
f(-1) -1
"""
    assert r == expect


def test_for_loop():
    r = run_example("for-loop.nc")
    expect = """\
1
4
9
16
y= 4
"""
    assert r == expect


def test_for_loop2():
    r = run_example("for-loop2.nc")
    expect = """\
foo
bar
baz
"""
    assert r == expect


def test_for_loop3():
    r = run_example("for-loop3.nc")
    expect = """\
1
2
3
4
5
6
7
8
9
10
"""
    assert r == expect


def test_function():
    r = run_example("function.nc")
    expect = "7\n"
    assert r == expect


def test_function2():
    r = run_example("function2.nc")
    expect = """\
this is a string not a comment
16
"""
    assert r == expect


def test_if():
    r = run_example("if.nc")
    expect = "42\n"
    assert r == expect


def test_list():
    r = run_example("list.nc")
    expect = "[-1, 2, 5]\n"
    assert r == expect


@pytest.mark.skip("TODO")
def test_logic_chain():
    r = run_example("logic-chain.nc")
    expect = """
1
True
"""
    assert r == expect


def test_nested_functions():
    r = run_example("nested-functions.nc")
    expect = """\
24
"""
    assert r == expect


def test_recursion():
    r = run_example("recursion.nc")
    expect = """\
0
1
2
"""
    assert r == expect


@pytest.mark.skip("TODO")
def test_rule110():
    r = run_example("rule110.nc")
    expect = """\
"""
    assert r == expect
