# Regression test for #1744: Space toggled a read-only checkbox (AnimatedCheckBox::keyPressEvent()), e.g. a
# disabled Boolean parameter, although a click doesn't.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtGui, QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")

g = app.createNode("fr.inria.built-in.Group")
check(g is not None, "createNode(Group) failed")
readOnly = g.createBooleanParam("natronTestReadOnly", "Read-only")
readOnly.setHelp("natronTestReadOnlyHelp")
readOnly.setEnabled(False)
editable = g.createBooleanParam("natronTestEditable", "Editable")
editable.setHelp("natronTestEditableHelp")
g.refreshUserParamsGUI()
app.clearSelection()
app.selectNode(g, True)
NatronGuiTest.sendKey("NodeGraph", "Return")  # open its settings panel
wait_idle()


def checkbox(help):
    boxes = [w for w in QtWidgets.QApplication.allWidgets()
             if w.metaObject().className() == "Natron::AnimatedCheckBox" and help in w.toolTip()]
    check(len(boxes) == 1, "%d checkboxes with the tooltip %s" % (len(boxes), help))
    return boxes[0]


def press_space(widget):
    QtWidgets.QApplication.sendEvent(widget, QtGui.QKeyEvent(QtCore.QEvent.KeyPress, QtCore.Qt.Key_Space, QtCore.Qt.NoModifier, " "))
    QtWidgets.QApplication.sendEvent(widget, QtGui.QKeyEvent(QtCore.QEvent.KeyRelease, QtCore.Qt.Key_Space, QtCore.Qt.NoModifier, " "))
    wait_idle()


# the editable one toggles: the key reaches the checkbox
press_space(checkbox("natronTestEditableHelp"))
check(editable.getValue(), "Space did not toggle an editable checkbox")
press_space(checkbox("natronTestReadOnlyHelp"))
check(not readOnly.getValue(), "Space toggled a read-only checkbox")
