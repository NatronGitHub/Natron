# Regression test for #1554: Natron exits cleanly with two viewers in one pane. Once the main window was
# deleted when its instance quits, deleting the first viewer made its pane show the other one, which was
# being deleted too.
import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
viewer.connectInput(0, gen)
viewer2 = app.createNode("fr.inria.built-in.Viewer")
check(viewer2 is not None, "no second viewer")
viewer2.connectInput(0, gen)
wait_idle()
check(app.getViewer(viewer2.getScriptName()) is not None, "no tab for the second viewer")
# the settings panel of the second viewer, open
app.clearSelection()
app.selectNode(viewer2, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
wait_idle()
print("viewers: %s %s" % (viewer.getScriptName(), viewer2.getScriptName()))
