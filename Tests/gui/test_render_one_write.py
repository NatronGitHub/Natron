# Rendering one Write node renders only that one (#815): the report saw other Write nodes fed by the
# same source write their files too, overwriting files that were not asked for. Three Write nodes
# share a Read; each is rendered in turn, with its Render button (pressed from the script) and with
# app.render(), and only its own files must be written. Two of them are created by copying the
# first one, as in the report ("copy-pasted and duplicated" Write nodes).
import os
import shutil
import tempfile

import NatronGuiTest
from natrontest import *

from run_gui_smoke_test import writePNG

if not os.environ.get("OCIO"):
    # the Write node fails without an OCIO config ("Invalid OCIO config")
    NatronGuiTest.skip("OCIO is not set to an OpenColorIO config")
ids = natron.getPluginIDs()
for pluginID in ("fr.inria.openfx.ReadPNG", "fr.inria.openfx.WritePNG"):
    if not any(i.startswith(pluginID) for i in ids):
        NatronGuiTest.skip("plug-in %s not found (OFX_PLUGIN_PATH=%s)" % (pluginID, os.environ.get("OFX_PLUGIN_PATH", "")))

FRAMES = (1, 2)

outDir = tempfile.mkdtemp(prefix="natron-render-one-write-")
try:
    app.getProjectParam("frameRange").set(FRAMES[0], FRAMES[-1])
    writePNG(os.path.join(outDir, "input.png"), 64, 48)
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(os.path.join(outDir, "input.png"))
    first = app.createNode("fr.inria.built-in.Write")
    first.connectInput(0, read)
    first.getParam("filename").set(os.path.join(outDir, "w0_###.png"))
    wait_idle()
    writes = [first]
    for i in (1, 2):
        # a copy of the first Write node, with its own file name
        app.selectNode(first, True)
        app.copySelectedNodes()
        app.pasteNodes()
        wait_idle()
        names = [w.getScriptName() for w in writes]
        selected = app.getSelectedNodes()
        copies = [n for n in selected if n.getScriptName() not in names]
        check(len(copies) == 1, "copy %d of %s: selected nodes after the paste: %r" % (i, first.getScriptName(), [n.getScriptName() for n in selected]))
        copy = copies[0]
        copy.connectInput(0, read)
        copy.getParam("filename").set(os.path.join(outDir, "w%d_###.png" % i))
        writes.append(copy)
    wait_idle()
    for w in writes:
        w.getParam("frameRange").set(2)  # manual
        wait_idle()
        w.getParam("firstFrame").set(FRAMES[0])
        w.getParam("lastFrame").set(FRAMES[-1])
    wait_idle()

    for mode in ("button", "apprender"):
        for i, w in enumerate(writes):
            for f in os.listdir(outDir):
                if f.startswith("w") and f.endswith(".png"):
                    os.remove(os.path.join(outDir, f))
            if mode == "button":
                w.getParam("startRender").trigger()
            else:
                app.render(w, FRAMES[0], FRAMES[-1])
            check(app.waitForRenders(60), "%s %s: the render did not finish in 60 s" % (mode, w.getScriptName()))
            wait_idle()
            written = sorted(f for f in os.listdir(outDir) if f.startswith("w") and f.endswith(".png"))
            expected = ["w%d_%03d.png" % (i, f) for f in FRAMES]
            check(written == expected, "%s %s: files written %r, expected %r" % (mode, w.getScriptName(), written, expected))
finally:
    shutil.rmtree(outDir, ignore_errors=True)
