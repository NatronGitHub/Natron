# A backdrop keeps the size the user gives it when its label changes: the label wraps to the
# backdrop's width (#626). Its label editor accepts a label without Natron's full font tag (#1807).
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtGui, QtWidgets
    except ImportError:
        QtWidgets = None

BACKDROP = "fr.inria.built-in.BackDrop"

bd = app.createNode(BACKDROP)
check(bd is not None, "createNode(%s) failed" % BACKDROP)
wait_idle()
label = bd.getParam("Label")
check(label is not None, "no Label parameter on %s" % bd.getScriptName())

bd.setSize(300, 200)
wait_idle()
size = bd.getSize()
check_close(size[0], 300, 0.5, "width after setSize()")
check_close(size[1], 200, 0.5, "height after setSize()")

# a long label in a big font: the backdrop used to grow to fit it, and never shrink back
label.setValue("<font size=\"7\">" + " ".join(["a long sticky note"] * 20) + "</font>")
wait_idle()
size = bd.getSize()
check_close(size[0], 300, 0.5, "width after a long label")
check_close(size[1], 200, 0.5, "height after a long label")

# the user can still make it smaller than its label
bd.setSize(150, 100)
wait_idle()
size = bd.getSize()
check_close(size[0], 150, 0.5, "width after setSize() smaller than the label")
check_close(size[1], 100, 0.5, "height after setSize() smaller than the label")

# but not to a negative width
bd.setSize(-10, 100)
wait_idle()
check(bd.getSize()[0] > 0, "width after setSize(-10, 100): %r" % (bd.getSize(),))


# #1807: the label editor asserted (Debug) or garbled the label (Release) when the label had no font tag
# that Natron writes, <font size="..." color="..." face="...">, or only part of it
if QtWidgets is None:
    NatronGuiTest.skip("PySide2/PySide6 not available: the label editor is not tested")


def editor_widget(widget_class, class_name=None, tooltip=None):
    """The widget of a class in the label editor of the backdrop's settings panel."""
    # The editor is the widget named after the param. Only allWidgets(): PySide2 invalidates the children
    # that findChildren() returns once their parent's wrapper is gone.
    widgets = QtWidgets.QApplication.allWidgets()
    editors = [w for w in widgets if w.objectName() == label.getScriptName()
               and any(isinstance(c, QtWidgets.QFontComboBox) and w.isAncestorOf(c) for c in widgets)]
    check(len(editors) == 1, "%d label editors" % len(editors))
    found = [w for w in widgets
             if isinstance(w, widget_class) and editors[0].isAncestorOf(w)
             and (class_name is None or w.metaObject().className() == class_name)
             and (tooltip is None or tooltip in w.toolTip())]
    check(len(found) == 1, "%d %s widgets in the label editor" % (len(found), class_name or widget_class))
    return found[0]


def set_font_size(size):
    """Type a size in the font size spinbox and press Return, as the user does."""
    spinbox = editor_widget(QtWidgets.QLineEdit, "Natron::SpinBox")
    spinbox.setText(str(size))
    for event_type in (QtCore.QEvent.KeyPress, QtCore.QEvent.KeyRelease):
        event = QtGui.QKeyEvent(event_type, QtCore.Qt.Key_Return, QtCore.Qt.NoModifier)
        QtWidgets.QApplication.sendEvent(spinbox, event)
    wait_idle()


def check_label(expected, what):
    """Check that the label is the expected string."""
    check(label.getValue() == expected, "%s: the label is %r, not %r" % (what, label.getValue(), expected))


text = "a sticky note"
label.setValue('<font size="7">' + text + '</font>')  # no color, no face
wait_idle()
app.clearSelection()
app.selectNode(bd, True)
NatronGuiTest.sendKey("NodeGraph", "Return")  # open its settings panel: it reads the font from the label
wait_idle()

combo = editor_widget(QtWidgets.QFontComboBox)
families = [combo.itemText(i) for i in range(combo.count()) if combo.itemText(i) != combo.currentFont().family()]
check(len(families) > 0, "only one font")
combo.setCurrentFont(QtGui.QFont(families[0]))
wait_idle()
family = combo.currentFont().family()
check_label('<font size="7" face="%s">%s</font>' % (family, text), "after choosing a font")

set_font_size(9)
check_label('<font size="9" face="%s">%s</font>' % (family, text), "after changing the font size")

# a label without any font tag, e.g. from Python while the panel is open
label.setValue(text)
wait_idle()
set_font_size(11)
check_label('<font size="11" color="#000000" face="%s">%s</font>' % (family, text),
            "after changing the size of a label without a font tag")

label.setValue(text)
wait_idle()
editor_widget(QtWidgets.QPushButton, tooltip="Italic").click()
wait_idle()
check_label("<i>" + text + "</i>", "after italic on a label without a font tag")

label.setValue(text)
wait_idle()
editor_widget(QtWidgets.QPushButton, tooltip="Bold").click()
wait_idle()
check_label("<b>" + text + "</b>", "after bold on a label without a font tag")
