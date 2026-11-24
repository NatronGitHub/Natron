# Regression test for #1554: app.closeProject() with nodes, then a nested event loop (waitIdle()) in the
# same script. The instance quits once its nodes stopped processing; it used to quit inside that nested
# loop, deleting the window under the script. The test passes only if Natron then exits cleanly.
import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
viewer.connectInput(0, gen)
fit_viewer()

NatronGuiTest.setDialogAnswers(["discard"])  # "Save changes?" if it is asked
print("closeProject() -> %r" % app.closeProject())
wait_idle()
wait_idle()
# the project is being closed: no node may be created in it
check(app.createNode(GENERATOR) is None, "createNode() worked in a project being closed")
print("dialogs: %r" % NatronGuiTest.getDialogLog())
