# App.timelineSetTime(frame) moves the current frame, also in background mode (#1796).
# Run by NatronRenderer -t (see Tests/CMakeLists.txt).
import traceback

from NatronEngine import natron


def main():
    status = 1
    try:
        app = natron.getInstance(0)
        node = app.createNode("fr.inria.built-in.Dot")
        assert node is not None, "createNode(Dot) failed"
        param = node.createIntParam("frameValue", "Frame")
        assert param.setExpression("frame", False), "setExpression() failed"
        for frame in (17, 1, -3, 1000):
            app.timelineSetTime(frame)
            assert app.timelineGetTime() == frame, (app.timelineGetTime(), frame)
            assert node.getCurrentTime() == frame, (node.getCurrentTime(), frame)
            assert param.getValue() == frame, (param.getValue(), frame)
        # the bounds are the project's frame range
        app.getProjectParam("frameRange").set(5, 20)
        assert (app.timelineGetLeftBound(), app.timelineGetRightBound()) == (5, 20)
        status = 0
    except BaseException:
        traceback.print_exc()
    natron.quit(status)


main()
