# Regression test for #1567: app.closeProject() on a project with unsaved changes and an open settings panel,
# answering "Discard" to "Save changes?", then the end of the test. The end of the test asks Natron to quit
# (as natron.quit() does), which used to close the instance a second time while it was already quitting
# (from the events processed by its first quit): the second quit deleted the settings panels again, and
# Natron crashed in KnobHolder::discardPanelPointer(). The test passes only if Natron then exits cleanly.
# Calling app.closeProject() or app.resetProject() again before the instance quits must not ask again whether
# to save the changes.
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

# "cancel" would answer a second "Save changes?" dialog, which must not be shown
NatronGuiTest.setDialogAnswers(["discard", "cancel", "cancel"])
r = app.closeProject()
log = list(NatronGuiTest.getDialogLog())
print("closeProject() -> %r, dialogs %r" % (r, log))
check(r, "closeProject() returned %r" % r)
check(len(log) == 1 and log[0].get("answer") == "discard", "expected one \"Save changes?\" dialog: %r" % log)

# the instance is quitting: closing it again succeeds and resetting it fails, without asking again to save
r2 = app.closeProject()
r3 = app.resetProject()
log = NatronGuiTest.getDialogLog()
print("second closeProject() -> %r, resetProject() -> %r, dialogs %r" % (r2, r3, log))
check(r2, "second closeProject() returned %r" % r2)
check(not r3, "resetProject() of a closing project returned %r" % r3)
check(len(log) == 1, "a second \"Save changes?\" dialog was shown: %r" % log)
