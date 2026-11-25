"""The ExprUtils noise functions and App.render(tasks) check their arguments (#1795).

They accept any sequence and raise TypeError for a wrong argument, instead of reading garbage or
crashing. Run by NatronRenderer -t (see Tests/CMakeLists.txt).
"""
import traceback

from NatronEngine import ExprUtils, natron


def assert_raises_type_error(func, *args):
    """Checks that func(*args) raises TypeError."""
    try:
        func(*args)
    except TypeError:
        return
    raise AssertionError("%s%r did not raise TypeError" % (func.__name__, args))


def check_noise(node):
    """The noise functions take a tuple, a list or a Double*Tuple, and reject anything else."""
    for point in ((0.3, 1.7), (0.3, 1.7, 2.1), (0.3, 1.7, 2.1, 0.6)):
        expected = ExprUtils.noise(point)
        assert ExprUtils.noise(list(point)) == expected, point
    p3 = node.createDouble3DParam("p3", "P3")
    p3.set(0.3, 1.7, 2.1)
    point3 = p3.get()
    # a Double3DTuple is iterable: its __getitem__ raises IndexError after the last item
    x, y, z = point3
    assert (x, y, z) == (0.3, 1.7, 2.1), (x, y, z)
    assert ExprUtils.noise(point3) == ExprUtils.noise((0.3, 1.7, 2.1))
    assert ExprUtils.fbm(point3) == ExprUtils.fbm([0.3, 1.7, 2.1])
    assert ExprUtils.vnoise([0.3, 1.7, 2.1]) == ExprUtils.vnoise((0.3, 1.7, 2.1))
    assert ExprUtils.snoise4([0.3, 1.7, 2.1, 0.6]) == ExprUtils.snoise4((0.3, 1.7, 2.1, 0.6))
    assert ExprUtils.pnoise([0.3, 1.7, 2.1], [4, 4, 4]) == ExprUtils.pnoise((0.3, 1.7, 2.1), (4, 4, 4))
    # snoise is static, like the other functions
    assert -1. <= ExprUtils.snoise((0.3, 1.7, 2.1)) <= 1.

    assert_raises_type_error(ExprUtils.noise, [1.0])
    assert_raises_type_error(ExprUtils.noise, [1.0, 2.0, 3.0, 4.0, 5.0])
    assert_raises_type_error(ExprUtils.noise, (1.0, "a"))
    assert_raises_type_error(ExprUtils.noise, node)
    assert_raises_type_error(ExprUtils.fbm, [1.0, 2.0])
    assert_raises_type_error(ExprUtils.cfbm4, (1.0, 2.0, 3.0))
    assert_raises_type_error(ExprUtils.cellnoise, 3.0)
    assert_raises_type_error(ExprUtils.pnoise, (1.0, 2.0, 3.0), (4, 4))
    assert_raises_type_error(ExprUtils.pnoise, (1.0, 2.0, 3.0), None)


def check_render_tasks(app, node):
    """App.render(tasks) takes tasks that are any sequence, and rejects a wrong task."""
    # not a Write node: render() prints "Invalid write node" and returns, but it reads the list task
    app.render([[node, 1, 1]])
    app.render([[node, 1, 1, 1]])
    assert_raises_type_error(app.render, [("Write1", 1, 1)])
    assert_raises_type_error(app.render, [(None, 1, 1)])
    assert_raises_type_error(app.render, [(node, "a", 1)])
    assert_raises_type_error(app.render, [(node, 1j, 1)])
    assert_raises_type_error(app.render, [(node, 1)])
    assert_raises_type_error(app.render, [5])


def main():
    status = 1
    try:
        app = natron.getInstance(0)
        node = app.createNode("fr.inria.built-in.Dot")
        assert node is not None, "createNode(Dot) failed"
        check_noise(node)
        check_render_tasks(app, node)
        status = 0
    except BaseException:
        traceback.print_exc()
    natron.quit(status)


main()
