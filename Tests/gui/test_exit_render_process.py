# Regression test: Natron exits while a render still runs in a separate process. The destructor of the
# ProcessHandler killed the process, QProcess emitted finished() from there, and
# AppInstance::startNextQueuedRender() destroyed the ProcessHandler a second time (crash at exit).
# The test passes only if Natron then exits cleanly.
import os
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

NFRAMES = 400  # more than the process renders before Natron exits

# The render process loads the saved project, whose Write node calls an afterFrameRender callback that slows
# the render down: define it in an init.py found by the process through NATRON_PLUGIN_PATH.
# The directory is not removed: the process may still write to it when the test ends (it is killed at exit).
tmpDir = tempfile.mkdtemp(prefix="natron-exit-render-process-")
pyDir = os.path.join(tmpDir, "py")
outDir = os.path.join(tmpDir, "out")
os.mkdir(pyDir)
os.mkdir(outDir)
with open(os.path.join(pyDir, "init.py"), "w") as f:
    f.write("import time\n"
            "def exitRenderProcessAfterFrame(frame, thisNode, app):\n"
            "    time.sleep(0.25)\n"
            "import __main__\n"
            "__main__.exitRenderProcessAfterFrame = exitRenderProcessAfterFrame\n")
os.environ["NATRON_PLUGIN_PATH"] = pyDir + os.pathsep + os.environ.get("NATRON_PLUGIN_PATH", "")


def exitRenderProcessAfterFrame(frame, thisNode, app):
    pass


import __main__
__main__.exitRenderProcessAfterFrame = exitRenderProcessAfterFrame


def frames_written():
    return len(os.listdir(outDir))


p = natron.getSettings().getParam("renderNewProcess")
check(p is not None, "no setting renderNewProcess")
p.set(True)  # the test mode does not save the settings
NatronGuiTest.setDialogAnswers(["discard"] * 3)  # "Save changes?"

app.getProjectParam("frameRange").set(1, NFRAMES)
gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
write = app.createNode("fr.inria.built-in.Write")
check(write is not None, "createNode(Write) failed")
write.connectInput(0, gen)
write.getParam("filename").set(os.path.join(outDir, "frame_###.png"))
write.getParam("afterFrameRender").set("exitRenderProcessAfterFrame")
wait_idle(10)

app.render(write, 1, NFRAMES)  # returns at once: the process renders a saved copy of the project
t0 = time.time()
while time.time() - t0 < 60 and frames_written() < 2:
    NatronGuiTest.waitIdle(0.2)  # processes the events; it may time out while the process renders
n = frames_written()
print("frames written by the process before the exit: %d" % n)
check(n >= 2, "the render process did not write 2 frames in 60 s")
check(n < NFRAMES, "the render process finished before the exit: slow it down")
# the script ends here: Natron exits while the process renders
