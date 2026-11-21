# Renders started by Python, with Python render callbacks on the Write node (#1568): the Render button
# of a Write node triggered from a script, and app.render(). In the GUI both return at once, and
# app.waitForRenders() waits for the render while the callbacks run on the render threads.
# Tests/gui/run_renderer_callbacks_test.py tests the blocking renders of NatronRenderer -t.
import os
import shutil
import tempfile

import NatronGuiTest
from natrontest import *

from run_gui_smoke_test import writePNG

ids = natron.getPluginIDs()
for pluginID in ("fr.inria.openfx.ReadPNG", "fr.inria.openfx.WritePNG"):
    if not any(i.startswith(pluginID) for i in ids):
        NatronGuiTest.skip("plug-in %s not found (OFX_PLUGIN_PATH=%s)" % (pluginID, os.environ.get("OFX_PLUGIN_PATH", "")))

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
    # The Render button of a Write node renders the project frame range in the GUI, even with the
    # manual frame range of the Write node (not what this test checks): use the same range.
    app.getProjectParam("frameRange").set(1, 3)
    writePNG(os.path.join(outDir, "input.png"), 64, 48)
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
        if mode == "button":
            write.getParam("startRender").trigger()
        else:
            app.render(write, 1, 3)
        check(app.waitForRenders(120), "%s: the render did not finish in 120 s" % mode)
        wait_idle()
        written = sorted(f for f in os.listdir(outDir) if f.startswith(mode + "_"))
        expected = ["%s_%03d.png" % (mode, f) for f in (1, 2, 3)]
        check(written == expected, "%s: frames written %r, expected %r" % (mode, written, expected))
        check(calls.count(("beforeRender", write.getScriptName())) == 1, "%s: beforeRender should run once: %r" % (mode, calls))
        frames = sorted(c[1] for c in calls if c[0] == "afterFrameRender")
        check(frames == [1, 2, 3], "%s: afterFrameRender frames %r" % (mode, frames))
finally:
    shutil.rmtree(outDir, ignore_errors=True)
