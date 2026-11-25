# Renders started by Python, with Python render callbacks on the Write node (#1568): the Render button
# of a Write node triggered from a script, and app.render(). In the GUI both return at once, and
# app.waitForRenders() waits for the render while the callbacks run on the render threads.
# Tests/gui/run_renderer_callbacks_test.py tests the blocking renders of NatronRenderer -t.
import os
import shutil
import tempfile

from natrontest import *

from natronrunner import write_png

require_ocio()
require_plugin("fr.inria.openfx.ReadPNG")
require_plugin("fr.inria.openfx.WritePNG")

calls = []


def beforeRender(thisNode, app):
    calls.append(("beforeRender", thisNode.getScriptName()))


def afterFrame(frame, thisNode, app):
    calls.append(("afterFrameRender", int(frame)))


import __main__
__main__.renderCallbacksBefore = beforeRender
__main__.renderCallbacksAfterFrame = afterFrame

outDir = tempfile.mkdtemp(prefix="natron-render-callbacks-")
try:
    # The project frame range is not the one of the Write node: the Render button renders the Manual
    # frame range of the Write node, set before its file name (#1574).
    app.getProjectParam("frameRange").set(1, 5)
    write_png(os.path.join(outDir, "input.png"), 64, 48)
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(os.path.join(outDir, "input.png"))
    write = app.createNode("fr.inria.built-in.Write")
    write.connectInput(0, read)
    write.getParam("beforeRender").set("renderCallbacksBefore")
    write.getParam("afterFrameRender").set("renderCallbacksAfterFrame")
    write.getParam("frameRange").set(2)  # manual
    wait_idle()
    write.getParam("firstFrame").set(1)
    write.getParam("lastFrame").set(3)
    wait_idle()
    check((write.getParam("firstFrame").get(), write.getParam("lastFrame").get()) == (1, 3),
          "the frame range of the Write node is %r" % ((write.getParam("firstFrame").get(), write.getParam("lastFrame").get()),))

    for mode in ("button", "apprender"):
        del calls[:]
        write.getParam("filename").set(os.path.join(outDir, mode + "_###.png"))
        # the first file name creates the encoder (WritePNG), which used to reset the frame range (#1574)
        frameRange = tuple(write.getParam(p).get() for p in ("frameRange", "firstFrame", "lastFrame"))
        check(frameRange == (2, 1, 3), "%s: the frame range of the Write node after setting its file name is %r" % (mode, frameRange))
        check(write.getParam("firstFrame").getIsVisible() and write.getParam("lastFrame").getIsVisible(),
              "%s: First/Last Frame are hidden with a Manual frame range" % mode)
        if mode == "button":
            write.getParam("startRender").trigger()
        else:
            app.render(write, 1, 3)
        check(app.waitForRenders(60), "%s: the render did not finish in 60 s" % mode)
        wait_idle()
        written = sorted(f for f in os.listdir(outDir) if f.startswith(mode + "_"))
        expected = ["%s_%03d.png" % (mode, f) for f in (1, 2, 3)]
        check(written == expected, "%s: frames written %r, expected %r" % (mode, written, expected))
        check(calls.count(("beforeRender", write.getScriptName())) == 1, "%s: beforeRender should run once: %r" % (mode, calls))
        frames = sorted(c[1] for c in calls if c[0] == "afterFrameRender")
        check(frames == [1, 2, 3], "%s: afterFrameRender frames %r" % (mode, frames))
finally:
    shutil.rmtree(outDir, ignore_errors=True)
