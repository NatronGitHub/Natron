# ColorParam.get(frame) returns the four channels of an RGBA color at that frame; it returned the blue value
# as the alpha (#1712). Run by NatronRenderer -t (see Tests/CMakeLists.txt).
from NatronEngine import natron

status = 1
try:
    app = natron.getInstance(0)
    node = app.createNode("fr.inria.built-in.Dot")
    assert node is not None, "createNode(Dot) failed"
    param = node.createColorParam("color1712", "Color", True)
    assert param is not None, "createColorParam() failed"
    keys = {1: (0.1, 0.2, 0.3, 0.4), 10: (0.5, 0.6, 0.7, 0.8)}
    for frame, rgba in keys.items():
        for dim, value in enumerate(rgba):
            param.setValueAtTime(value, frame, dim)
    for frame, rgba in keys.items():
        c = param.get(frame)
        got = (c.r, c.g, c.b, c.a)
        assert all(abs(g - e) < 1e-6 for g, e in zip(got, rgba)), "get(%d) is %r, expected %r" % (frame, got, rgba)
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
