# Regression test for #1567: app.closeProject() on a project with unsaved changes and an open settings panel,
# answering "Discard" to "Save changes?", then the end of the test. The end of the test asks Natron to quit
# (as natron.quit() does), which used to close the instance a second time while it was already quitting
# (from the events processed by its first quit): the second quit deleted the settings panels again, and
# Natron crashed in KnobHolder::discardPanelPointer(). The test passes only if Natron then exits cleanly.
import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
viewer.connectInput(0, gen)
# the settings panel of the node, open in the Properties pane
app.clearSelection()
app.selectNode(gen, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
wait_idle()
# an unsaved change
app.getProjectParam("frameRange").set(1, 200)
wait_idle()

NatronGuiTest.setDialogAnswers(["discard"])
r = app.closeProject()
log = NatronGuiTest.getDialogLog()
print("closeProject() -> %r, dialogs %r" % (r, log))
check(r, "closeProject() returned %r" % r)
check(len(log) == 1 and log[0].get("answer") == "discard", "expected one \"Save changes?\" dialog: %r" % log)
