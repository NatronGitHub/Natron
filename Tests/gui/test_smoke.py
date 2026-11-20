# GUI smoke test: create and connect nodes, render in the viewer, delete a node and undo/redo it
# with the node graph shortcuts.
import sys

import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)

viewer = viewer_node(app)
renders = test_plugin_counter("Generator.render")

# create, connect, render
gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
name = gen.getScriptName()
viewer.connectInput(0, gen)
check(same_node(viewer.getInput(0), gen), "the viewer is not connected to %s" % name)
fit_viewer()
check(test_plugin_counter("Generator.render") > renders, "the viewer did not render %s" % name)

x, y = rod_center(gen)
r, g, b, a = NatronGuiTest.getDisplayedPixel("Viewer1", x, y)
check_close(r, srgb8(0.5), 3, "displayed red at the image centre")
check_close(g, srgb8(0.5), 3, "displayed green at the image centre")
check_close(b, srgb8(0.5), 3, "displayed blue at the image centre")

# delete with the node graph shortcut (an undoable command), then undo and redo it
app.clearSelection()
app.selectNode(gen, True)
NatronGuiTest.sendKey("NodeGraph", "Backspace")
wait_idle()
check(app.getNode(name) is None, "%s was not deleted" % name)

NatronGuiTest.sendKey("NodeGraph", "Ctrl+Z")
wait_idle()
gen = app.getNode(name)
check(gen is not None, "undo did not restore %s" % name)
check(same_node(viewer.getInput(0), gen), "undo did not reconnect %s to the viewer" % name)

# the platform's redo shortcut (QKeySequence::Redo)
NatronGuiTest.sendKey("NodeGraph", "Ctrl+Y" if sys.platform.startswith("win") else "Ctrl+Shift+Z")
wait_idle()
check(app.getNode(name) is None, "redo did not delete %s again" % name)

check(not NatronGuiTest.isRendering(), "a render is still running")
print("dialogs: %r" % NatronGuiTest.getDialogLog())
