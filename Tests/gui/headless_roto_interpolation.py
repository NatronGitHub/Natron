# BezierCurve.setKeyframeInterpolation() (#1749): run by NatronRenderer -t (see Tests/CMakeLists.txt).
"""Sets the interpolation of a Roto shape's keyframes and checks the shape between them."""

import traceback

import NatronEngine
from NatronEngine import natron

KeyframeType = NatronEngine.Natron.KeyframeTypeEnum

status = 1
try:
    app = natron.getInstance(0)
    # the Roto node needs the Roto plug-in of openfx-misc: without it, creating one fails
    if "net.sf.openfx.RotoPlugin" not in natron.getPluginIDs():
        print("no Roto plug-in: skipped")
        status = 77
    else:
        node = app.createNode("fr.inria.built-in.Roto")
        roto = node.getRotoContext()
        shape = roto.createBezier(0, 0, 0)
        for i in range(3):
            shape.addControlPoint(0, 0)
        # a square at frame 0, twice as large at frame 10 (two keyframes)
        for time, size in ((0, 10), (10, 20)):
            for index, (x, y) in enumerate(((0, 0), (size, 0), (size, size), (0, size))):
                shape.setPointAtIndex(index, time, x, y, x, y, x, y)
                shape.setFeatherPointAtIndex(index, time, x, y, x, y, x, y)
        assert shape.getKeyframeInterpolation(0) == KeyframeType.eKeyframeTypeSmooth
        assert shape.getKeyframeInterpolation(5) == KeyframeType.eKeyframeTypeNone
        assert shape.setKeyframeInterpolation(0, KeyframeType.eKeyframeTypeLinear)
        assert shape.setKeyframeInterpolation(10, KeyframeType.eKeyframeTypeLinear)
        assert shape.getKeyframeInterpolation(0) == KeyframeType.eKeyframeTypeLinear
        # linear: a quarter of the way between the keyframes, the second point is at
        # 12.5 (smooth keyframes give 11.5625)
        x = shape.getControlPointPosition(1, 2.5)[0]
        assert abs(x - 12.5) < 1e-9, x
        assert abs(shape.getFeatherPointPosition(1, 2.5)[0] - 12.5) < 1e-9
        # no keyframe there
        assert not shape.setKeyframeInterpolation(5, KeyframeType.eKeyframeTypeLinear)
        status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
