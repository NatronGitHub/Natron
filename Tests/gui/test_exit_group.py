# Regression test for #1554: Natron exits cleanly with a Group node whose node graph was never shown (it
# is not in a pane). Once the main window was deleted when its instance quits, QWidget's destructor
# deleted that node graph with a Gui pointer that the closing Gui had not discarded.
import NatronGuiTest
from natrontest import *

group = app.createNode("fr.inria.built-in.Group")
check(group is not None, "createNode(Group) failed")
wait_idle()
print("created %s" % group.getScriptName())
