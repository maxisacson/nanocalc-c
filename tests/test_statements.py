import pytest
import subprocess
import sys


def to_number(s):
    try:
        return int(s)
    except ValueError:
        return float(s)
    except Exception:
        print(s)
        raise


def eval_expression(expr):
    res = subprocess.run("../nc", input=expr, text=True, capture_output=True)
    if res.stderr:
        print(res.stderr, file=sys.stderr)
    return res.stdout


def eval_numeric_expression(expr):
    res = subprocess.run("../nc", input=expr, text=True, capture_output=True)
    if res.stderr:
        print(res.stderr, file=sys.stderr)
    return to_number(res.stdout.splitlines()[-1])


def eval_void_expression(expr):
    res = subprocess.run("../nc", input=expr, text=True, capture_output=True)
    if res.stderr:
        print(res.stderr, file=sys.stderr)


def test_assignment():
    code = "x = 14"

    v = eval_numeric_expression(code)

    assert v == 14


def test_block():
    code = """
    {
        x = 1
        y = 2
        x + 2*y
    }
    """

    v = eval_numeric_expression(code)

    assert v == 5


def test_cases():
    code = """
    x = -10
    {
        1 if x > 0
        -1 if x < 0
        0
    }
    """

    v = eval_numeric_expression(code)

    assert v == -1


def test_cases2():
    code = """
    x = 10
    {
        1 if x > 0
        -1 if x < 0
        0
    }
    """

    v = eval_numeric_expression(code)

    assert v == 1


def test_cases3():
    code = """
    x = 0
    {
        1 if x > 0
        -1 if x < 0
        0
    }
    """

    v = eval_numeric_expression(code)

    assert v == 0


def test_function():
    code = """
    f(x) = {
        x^2 + 2*x + 10
    }
    """

    v = eval_void_expression(code)

    assert v is None

    v = eval_numeric_expression(code + "f(0)")
    assert v == 10

    v = eval_numeric_expression(code + "f(3)")
    assert v == 25


def test_function2():
    code = """
    f(x) = {
        0 if x < 0
        x^2
    }
    x = [-2, -1, 0, 1, 2]
    table x f(x)
    """

    out = eval_expression(code)

    assert out == "-2 0\n-1 0\n0 0\n1 1\n2 4\n"


def test_function3():
    code = """
    f(x, y) = {
        x^2 + y^2
    }
    x = [-2, -1, 0, 1, 2]
    y = [1, 0, 2, 0, 3]
    f(x, y)
    """

    out = eval_expression(code)

    assert out == "[5, 1, 4, 1, 13]\n"


def test_loop():
    code = """
    x = ["foo", "bar", "baz"]
    for s in x print s
    """

    out = eval_expression(code)

    assert out == "foo\nbar\nbaz\n"


def test_loop2():
    code = """
    s = 0
    for i in 1..4
        s = s + i
    s
    """

    v = eval_numeric_expression(code)

    assert v == 10


def test_loop3():
    code = """
    s = 0
    for i in 1..4 {
        s = s + i
        print i
    }
    s
    """

    out = eval_expression(code)

    assert out == "1\n2\n3\n4\n10\n"


def test_nested_scopes():
    code = """
    a = 1
    f(x) = {
        b = 2
        g(y) = b * y
        a * g(x)
    }
    f(3)
    """
    v = eval_numeric_expression(code)

    assert v == 6


# def test_range():
#     code = "1..5"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [1, 2, 3, 4, 5]
#
#     assert actual == expected
#
#
# def test_range2():
#     code = "0..4..3"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [0, 2, 4]
#
#     assert actual == expected
#
#
# def test_range3():
#     code = "1..7..+2"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [1, 3, 5, 7]
#
#     assert actual == expected
#
#
# def test_range4():
#     code = "0..1..3"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [0.0, 0.5, 1.0]
#
#     assert actual == expected
#
#
# def test_range5():
#     code = "0...1..2"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [0.0, 1.0]
#
#     assert all(map(lambda x: isinstance(x, float), actual))
#     assert actual == expected
#
#
# def test_range6():
#     code = "0...1..+0.2"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
#
#     assert actual == pytest.approx(expected)
#
#
# def test_range7():
#     code = "x=0..1"
#
#     e = parse_expression(code)
#     actual = e.eval()
#
#     assert isinstance(actual, list)
#     expected = [0, 1]
#
#     assert actual == expected
#
#
# def test_range8():
#     code = "0..Inf..3"
#
#     e = parse_expression(code)
#     v = e.eval()
#
#     assert isinstance(v, types.GeneratorType)
#     actual = list(v)
#     expected = [0, 1, 2]
#
#     assert actual == expected
#
#
# def test_range_comp():
#     code = '1..3==1..3'
#
#     e = parse_expression(code)
#     actual = e.eval()
#     expected = [True, True, True]
#
#     assert actual == expected
#
#
# def test_range_comp2():
#     code = '1..3==2..4'
#
#     e = parse_expression(code)
#     actual = e.eval()
#     expected = [False, False, False]
#
#     assert actual == expected
#
#
# def test_range_comp3():
#     code = '1..3==[2, 2, 2]'
#
#     e = parse_expression(code)
#     actual = e.eval()
#     expected = [False, True, False]
#
#     assert actual == expected
