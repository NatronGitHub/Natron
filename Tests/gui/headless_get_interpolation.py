# AnimatedParam.getInterpolationAtTime(time, dimension=0) reads the interpolation that
# setInterpolationAtTime() sets (#1798). Run by NatronRenderer -t (see Tests/CMakeLists.txt).
import traceback

import NatronEngine
from NatronEngine import natron

KeyframeType = NatronEngine.Natron.KeyframeTypeEnum
TYPES = (
    KeyframeType.eKeyframeTypeConstant,
    KeyframeType.eKeyframeTypeLinear,
    KeyframeType.eKeyframeTypeSmooth,
    KeyframeType.eKeyframeTypeCatmullRom,
    KeyframeType.eKeyframeTypeCubic,
    KeyframeType.eKeyframeTypeHorizontal,
    KeyframeType.eKeyframeTypeFree,
    KeyframeType.eKeyframeTypeBroken,
)


def main():
    status = 1
    try:
        app = natron.getInstance(0)
        node = app.createNode("fr.inria.built-in.Dot")
        assert node is not None, "createNode(Dot) failed"
        param = node.createDoubleParam("interp", "Interp")
        times = [10. * (i + 1) for i in range(len(TYPES))]
        for i, time in enumerate(times):
            param.setValueAtTime(float(i), time)
        # new keyframes are smooth
        assert param.getInterpolationAtTime(times[0]) == KeyframeType.eKeyframeTypeSmooth
        for time, interpolation in zip(times, TYPES):
            assert param.setInterpolationAtTime(time, interpolation), time
        for time, interpolation in zip(times, TYPES):
            got = param.getInterpolationAtTime(time)
            assert got == interpolation, (time, got, interpolation)
            assert param.getInterpolationAtTime(time, 0) == interpolation, time
        # no keyframe at that time, or no such dimension
        assert param.getInterpolationAtTime(15.) == KeyframeType.eKeyframeTypeNone
        assert param.getInterpolationAtTime(times[0], 1) == KeyframeType.eKeyframeTypeNone
        assert param.getInterpolationAtTime(times[0], -1) == KeyframeType.eKeyframeTypeNone
        not_animated = node.createDoubleParam("still", "Still")
        assert not_animated.getInterpolationAtTime(times[0]) == KeyframeType.eKeyframeTypeNone
        status = 0
    except BaseException:
        traceback.print_exc()
    natron.quit(status)


main()
