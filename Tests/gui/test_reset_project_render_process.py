# Regression test for #1575: app.resetProject() while a render runs in a separate process, with "Append new
# renders to queue" on. The process renders a saved copy of the project and goes on after the reset; when it
# finished, its Progress panel task used the deleted Write node (crash in ProgressTaskInfo::cancelTask()). A
# render of the next project waits for the process, then runs, and app.waitForRenders() returns.
import os
import shutil
import tempfile
import time

import NatronGuiTest
from natrontest import *

if not os.environ.get("OCIO"):
    # the Write node fails without an OCIO config ("Invalid OCIO config")
    NatronGuiTest.skip("OCIO is not set to an OpenColorIO config")
require_plugin(GENERATOR)
if not any(i.startswith("fr.inria.openfx.WritePNG") for i in natron.getPluginIDs()):
    NatronGuiTest.skip("plug-in fr.inria.openfx.WritePNG not found (OFX_PLUGIN_PATH=%s)" % os.environ.get("OFX_PLUGIN_PATH", ""))

NFRAMES = 20

# The render process loads the saved project, whose Write node calls an afterFrameRender callback that slows
# the render down: define it in an init.py found by the process through NATRON_PLUGIN_PATH (it inherits the
# environment of this one).
tmpDir = tempfile.mkdtemp(prefix="natron-reset-project-render-process-")
pyDir = os.path.join(tmpDir, "py")
outDir = os.path.join(tmpDir, "out")
os.mkdir(pyDir)
os.mkdir(outDir)
with open(os.path.join(pyDir, "init.py"), "w") as f:
    f.write("import time\n"
            "def resetProjectProcessAfterFrame(frame, thisNode, app):\n"
            "    time.sleep(0.25)\n"
            "import __main__\n"
            "__main__.resetProjectProcessAfterFrame = resetProjectProcessAfterFrame\n")
os.environ["NATRON_PLUGIN_PATH"] = pyDir + os.pathsep + os.environ.get("NATRON_PLUGIN_PATH", "")


def resetProjectProcessAfterFrame(frame, thisNode, app):
    pass


import __main__
__main__.resetProjectProcessAfterFrame = resetProjectProcessAfterFrame


def set_setting(name, value):
    # the test mode does not save the settings
    p = natron.getSettings().getParam(name)
    check(p is not None, "no setting %s" % name)
    p.set(value)


def frames_written(prefix):
    return len([f for f in os.listdir(outDir) if f.startswith(prefix + "_")])


def process_events(seconds, until=None):
    t0 = time.time()
    while time.time() - t0 < seconds:
        if until is not None and until():
            return True
        NatronGuiTest.waitIdle(0.2)  # processes the events; it may time out while the process renders
    return until() if until is not None else True


def make_writer(prefix, nframes, callback):
    app.getProjectParam("frameRange").set(1, nframes)
    gen = app.createNode(GENERATOR)
    check(gen is not None, "createNode(%s) failed" % GENERATOR)
    write = app.createNode("fr.inria.built-in.Write")
    check(write is not None, "createNode(Write) failed")
    write.connectInput(0, gen)
    write.getParam("filename").set(os.path.join(outDir, prefix + "_###.png"))
    if callback:
        write.getParam("afterFrameRender").set("resetProjectProcessAfterFrame")
    wait_idle()
    return write


try:
    set_setting("queueRenders", True)
    set_setting("renderNewProcess", True)
    NatronGuiTest.setDialogAnswers(["discard"] * 3)  # "Save changes?"

    write = make_writer("process", NFRAMES, True)
    app.render(write, 1, NFRAMES)  # returns at once: the process renders a saved copy of the project
    check(process_events(120, lambda: frames_written("process") >= 2), "the render process did not write 2 frames in 120 s")
    print("frames written by the process before the reset: %d" % frames_written("process"))

    r = app.resetProject()
    print("resetProject() -> %r, frames written %d" % (r, frames_written("process")))
    check(r, "resetProject() returned %r" % r)
    check(frames_written("process") < NFRAMES, "the render process finished before the reset: slow it down")

    # a render of the next project, run in a separate process too: queued behind the first one
    write2 = make_writer("next", 3, False)
    app.render(write2, 1, 3)
    t0 = time.time()
    ok = app.waitForRenders(120)
    print("waitForRenders() -> %r in %.1f s, frames written: process %d, next %d"
          % (ok, time.time() - t0, frames_written("process"), frames_written("next")))
    process_events(1)  # the signals of the processes that just finished
    check(ok, "app.waitForRenders() timed out")
    check(frames_written("process") == NFRAMES, "the render process did not go on after the reset")
    check(frames_written("next") == 3, "the render of the next project did not run")
finally:
    shutil.rmtree(tmpDir, ignore_errors=True)
