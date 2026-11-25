# Rendering one Write node renders only that one (#815): the report saw other Write nodes fed by the
# same source write their files too, overwriting files that were not asked for. Three Write nodes
# share a Read; each is rendered in turn, with its Render button (pressed from the script) and with
# app.render(), and only its own files must be written. Two of them are created by copying the
# first one, as in the report ("copy-pasted and duplicated" Write nodes). The project frame range is the
# range rendered. Render > Render Selected (F7), which renders every selected Write node, is not tested.
import os
import shutil
import tempfile

from natrontest import *

import natronrunner

require_ocio()
require_plugin("fr.inria.openfx.ReadPNG")
require_plugin("fr.inria.openfx.WritePNG")

FRAMES = (1, 2)

outDir = tempfile.mkdtemp(prefix="natron-render-one-write-")
try:
    app.getProjectParam("frameRange").set(FRAMES[0], FRAMES[-1])
    natronrunner.write_png(os.path.join(outDir, "input.png"), 64, 48)
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(os.path.join(outDir, "input.png"))
    first = app.createNode("fr.inria.built-in.Write")
    first.connectInput(0, read)
    first.getParam("filename").set(os.path.join(outDir, "w0_###.png"))
    wait_idle()
    writes = [first]
    for i in (1, 2):
        # a copy of the first Write node, with its own file name
        # a pasted Write node (a group) is not selected: find it among the new nodes
        before = set(n.getScriptName() for n in app.getChildren())
        app.selectNode(first, True)
        app.copySelectedNodes()  # through the clipboard
        app.pasteNodes()
        wait_idle()
        copies = [n for n in app.getChildren() if n.getScriptName() not in before]
        check(len(copies) == 1, "copy %d of %s: nodes added by the paste (none: clipboard?): %r" %
              (i, first.getScriptName(), [n.getScriptName() for n in copies]))
        copy = copies[0]
        copy.connectInput(0, read)
        copy.getParam("filename").set(os.path.join(outDir, "w%d_###.png" % i))
        writes.append(copy)
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
