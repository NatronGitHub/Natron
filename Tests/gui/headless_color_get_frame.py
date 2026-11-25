"""ColorParam.get(frame) returns the four channels of an RGBA color at that frame (#1712).

It returned the blue value as the alpha.
"""

import natronheadless


def check(param, frame, rgba):
    c = param.get(frame)
    got = (c.r, c.g, c.b, c.a)
    assert all(abs(g - e) < 1e-6 for g, e in zip(got, rgba)), "get(%d) is %r, expected %r" % (frame, got, rgba)


def main(app):
    node = app.createNode("fr.inria.built-in.Dot")
    assert node is not None, "createNode(Dot) failed"
    # not animated
    static = node.createColorParam("static1712", "Static", True)
    assert static is not None, "createColorParam() failed"
    static.set(0.1, 0.2, 0.3, 0.4)
    for frame in (1, 10):
        check(static, frame, (0.1, 0.2, 0.3, 0.4))
    # animated: the keyframes made a Debug NatronRenderer die with SIGFPE on Linux (#1753)
    animated = node.createColorParam("animated1712", "Animated", True)
    assert animated is not None, "createColorParam() failed"
    keys = {1: (0.1, 0.2, 0.3, 0.4), 10: (0.5, 0.6, 0.7, 0.8)}
    for frame, (r, g, b, a) in keys.items():
        animated.set(r, g, b, a, frame)
    for frame, rgba in keys.items():
        check(animated, frame, rgba)


natronheadless.run(main)
