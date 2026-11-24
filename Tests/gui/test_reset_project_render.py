# Regression test for #1575: app.resetProject() and app.closeProject() while a render started from Python
# runs, with a Python render callback (afterFrameRender) on the Write node. Closing the project stops the
# render and waits for it; the main thread used to wait with the Python GIL held, while the render thread
# waited for the GIL to run the callback: Natron hung.
import faulthandler
import os
import shutil
import sys
import tempfile
import time

import NatronGuiTest
from natrontest import *

if not os.environ.get("OCIO"):
    # the Write node fails without an OCIO config ("Invalid OCIO config")
    NatronGuiTest.skip("OCIO is not set to an OpenColorIO config")
require_plugin(GENERATOR)
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    NatronGuiTest.skip("plug-in fr.inria.openfx.WritePNG not found (OFX_PLUGIN_PATH=%s)" % os.environ.get("OFX_PLUGIN_PATH", ""))

NFRAMES = 100
calls = []


def afterFrame(frame, thisNode, app):
    # runs on a render thread, with the GIL; time.sleep() releases it, and takes it again when it returns
    calls.append(int(frame))
    time.sleep(0.2)
    calls.append(-int(frame))


import __main__
__main__.resetProjectRenderAfterFrame = afterFrame

outDir = tempfile.mkdtemp(prefix="natron-reset-project-render-")


def start_render(prefix):
    app.getProjectParam("frameRange").set(1, NFRAMES)
    gen = app.createNode(GENERATOR)
    check(gen is not None, "createNode(%s) failed" % GENERATOR)
    write = app.createNode("fr.inria.built-in.Write")
    check(write is not None, "createNode(Write) failed")
    write.connectInput(0, gen)
    write.getParam("filename").set(os.path.join(outDir, prefix + "_###.png"))
    write.getParam("afterFrameRender").set("resetProjectRenderAfterFrame")
    wait_idle()
    del calls[:]
    app.render(write, 1, NFRAMES)  # returns at once in the GUI
    # wait (without the GIL: time.sleep() releases it) until the render thread is in the callback
    t0 = time.time()
    while not calls and time.time() - t0 < 60:
        time.sleep(0.01)
    check(calls, "%s: the render did not call afterFrameRender in 60 s" % prefix)


def frames_written(prefix):
    return len([f for f in os.listdir(outDir) if f.startswith(prefix + "_")])


# If the main thread hangs with the GIL held, no Python thread can run: faulthandler's watchdog is a C thread.
TIMEOUT = 60
try:
    NatronGuiTest.setDialogAnswers(["discard", "discard"])  # "Save changes?"

    start_render("reset")
    faulthandler.dump_traceback_later(TIMEOUT, exit=True, file=sys.__stderr__)
    t0 = time.time()
    r = app.resetProject()
    dt = time.time() - t0
    faulthandler.cancel_dump_traceback_later()
    print("resetProject() -> %r in %.2f s, frames written %d, dialogs %r" % (r, dt, frames_written("reset"), NatronGuiTest.getDialogLog()))
    check(r, "resetProject() returned %r" % r)
    check(dt < TIMEOUT, "resetProject() took %.1f s" % dt)
    check(frames_written("reset") < NFRAMES, "the render was not stopped by resetProject()")
    wait_idle()
    check(app.getNode("Write1") is None, "the Write node is still in the project after resetProject()")

    # app.closeProject() stops the render too, then quits this instance (the last one: Natron exits once the
    # test ends, and the exit status of the test is that of the script)
    start_render("close")
    faulthandler.dump_traceback_later(TIMEOUT, exit=True, file=sys.__stderr__)
    t0 = time.time()
    r = app.closeProject()
    dt = time.time() - t0
    faulthandler.cancel_dump_traceback_later()
    print("closeProject() -> %r in %.2f s, dialogs %r" % (r, dt, NatronGuiTest.getDialogLog()))
    check(r, "closeProject() returned %r" % r)
    check(dt < TIMEOUT, "closeProject() took %.1f s" % dt)
    # the nodes stop processing (the render is aborted), then the instance quits once the test ends
    wait_idle()
    check(frames_written("close") < NFRAMES, "the render was not stopped by closeProject()")
finally:
    faulthandler.cancel_dump_traceback_later()
    shutil.rmtree(outDir, ignore_errors=True)
