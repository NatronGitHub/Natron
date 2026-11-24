# AnimatedParam.setDerivativesAtTime() (#1745): run by NatronRenderer -t (see Tests/CMakeLists.txt).
"""Sets the tangents of keyframes and checks the interpolated values."""

import traceback

import NatronEngine
from NatronEngine import natron


def _hermite(t, t0, v0, d0, t1, v1, d1):
    """The cubic Hermite interpolation between two keyframes."""
    s = (t - t0) / (t1 - t0)
    dt = t1 - t0
    return ((2 * s**3 - 3 * s**2 + 1) * v0 + (s**3 - 2 * s**2 + s) * dt * d0
            + (-2 * s**3 + 3 * s**2) * v1 + (s**3 - s**2) * dt * d1)


status = 1
try:
    app = natron.getInstance(0)
    group = app.createNode("fr.inria.built-in.Group")
    param = group.createDoubleParam("curve", "Curve")
    param.setValueAtTime(0, 0)
    param.setValueAtTime(10, 10)
    smooth = param.getValueAtTime(5)
    # right derivative 3 after the first key, left derivative 1 before the second one
    assert param.setDerivativesAtTime(0, 0, 3)
    assert param.setDerivativesAtTime(10, 1, 0)
    for t in (2.5, 5, 7.5):
        expected = _hermite(t, 0, 0, 3, 10, 10, 1)
        got = param.getValueAtTime(t)
        assert abs(got - expected) < 1e-9, (t, got, expected)
    assert param.getValueAtTime(5) != smooth
    # no keyframe there, no such dimension, not a floating-point parameter
    assert not param.setDerivativesAtTime(5, 1, 1)
    assert not param.setDerivativesAtTime(0, 1, 1, 1)
    count = group.createIntParam("count", "Count")
    count.setValueAtTime(1, 0)
    assert not count.setDerivativesAtTime(0, 1, 1)
    # the tangents are kept when another keyframe is added: the keys are free
    param.setValueAtTime(20, 30)
    assert abs(param.getValueAtTime(5) - _hermite(5, 0, 0, 3, 10, 10, 1)) < 1e-9
    status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
