# Regression test for #1744: Space toggled a read-only checkbox (AnimatedCheckBox::keyPressEvent()),
# e.g. a disabled Boolean parameter, although a click doesn't.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtGui, QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")


def find_checkbox(param_name):
    """The checkbox of a Boolean parameter in the open settings panels, by its paramName property."""
    boxes = [w for w in QtWidgets.QApplication.allWidgets()
             if w.property("paramName") == param_name
             and w.metaObject().className() == "Natron::AnimatedCheckBox"]
    check(len(boxes) == 1, "%d checkboxes for %s" % (len(boxes), param_name))
    return boxes[0]


def press_space(widget):
    """Send Space to the widget, without the keyboard focus (NATRON_TEST_NO_FOCUS)."""
    for event_type in (QtCore.QEvent.KeyPress, QtCore.QEvent.KeyRelease):
        event = QtGui.QKeyEvent(event_type, QtCore.Qt.Key_Space, QtCore.Qt.NoModifier, " ")
        QtWidgets.QApplication.sendEvent(widget, event)
    wait_idle()


g = app.createNode("fr.inria.built-in.Group")
check(g is not None, "createNode(Group) failed")
read_only = g.createBooleanParam("natronTestReadOnly", "Read-only")
read_only.setEnabled(False)
editable = g.createBooleanParam("natronTestEditable", "Editable")
g.refreshUserParamsGUI()
app.clearSelection()
app.selectNode(g, True)
NatronGuiTest.sendKey("NodeGraph", "Return")  # open its settings panel
wait_idle()

# the editable one toggles: the key reaches the checkbox
press_space(find_checkbox("natronTestEditable"))
check(editable.getValue(), "Space did not toggle an editable checkbox")
press_space(find_checkbox("natronTestReadOnly"))
check(not read_only.getValue(), "Space toggled a read-only checkbox")
