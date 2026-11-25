"""App.timelineSetTime(frame) moves the current frame, also in background mode (#1796).
"""

import natronheadless


def main(app):
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


natronheadless.run(main)
