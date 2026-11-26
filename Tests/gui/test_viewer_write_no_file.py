# The Viewer shows a Write node without writing its files (#815): viewing a Write node, or a node
# after it (a Dot here), rendered its writer, which wrote the file of the frame shown, so files that were never
# asked for were overwritten. The Viewer must show the input of the Write node and write nothing;
# the Render button of the Write node must still write its file.
import os
import shutil
import tempfile

import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)
require_ocio()
require_plugin("fr.inria.openfx.WritePNG")

outDir = tempfile.mkdtemp(prefix="natron-viewer-write-no-file-")
try:
    app.getProjectParam("frameRange").set(1, 1)
    viewer = viewer_node(app)
    gen = app.createNode(GENERATOR)
    write = app.createNode("fr.inria.built-in.Write")
    check(write.connectInput(0, gen), "the Write node cannot be connected to the generator")
    write.getParam("filename").set(os.path.join(outDir, "viewed_###.png"))
    check(viewer.connectInput(0, write), "the Viewer cannot be connected to the Write node")
    fit_viewer()
    app.getViewer("Viewer1").renderCurrentFrame(True)
    wait_idle()
    x, y = rod_center(gen)
    r, g, b, a = NatronGuiTest.getDisplayedPixel("Viewer1", x, y)
    print("displayed %r" % ((r, g, b, a),))
    # (the same before the fix: the writer copied its input to its output; only the files tell the difference)
    check_close(r, srgb8(0.5), 3, "displayed red (the input of the Write node)")
    written = sorted(os.listdir(outDir))
    check(written == [], "the Viewer wrote %r: viewing a Write node must not write its files" % written)

    # the Viewer showing a node after a Write node: another Write node, whose image the first render did not
    # cache, with its own file name
    write2 = app.createNode("fr.inria.built-in.Write")
    check(write2.connectInput(0, gen), "the second Write node cannot be connected to the generator")
    write2.getParam("filename").set(os.path.join(outDir, "after_###.png"))
    dot = app.createNode("fr.inria.built-in.Dot")
    check(dot.connectInput(0, write2), "the Dot cannot be connected to the second Write node")
    viewer.disconnectInput(0)
    check(viewer.connectInput(0, dot), "the Viewer cannot be connected to the Dot")
    app.getViewer("Viewer1").renderCurrentFrame(True)
    wait_idle()
    written = sorted(os.listdir(outDir))
    check(written == [], "the Viewer wrote %r: viewing a node after a Write node must not write its files" % written)

    write.getParam("startRender").trigger()
    check(app.waitForRenders(60), "the render did not finish in 60 s")
    wait_idle()
    written = sorted(os.listdir(outDir))
    check(written == ["viewed_001.png"], "the Render button wrote %r, expected ['viewed_001.png']" % written)
finally:
    shutil.rmtree(outDir, ignore_errors=True)
