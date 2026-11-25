# The label editor of a backdrop shows the bold and italic state of a label without Natron's font tag (e.g.
# set from Python), and keeps it when the text is edited: it used to show Bold checked but drop the bold
# when the text was edited (#1807).
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtWidgets

bd = app.createNode("fr.inria.built-in.BackDrop")
check(bd is not None, "createNode(BackDrop) failed")
label = bd.getParam("Label")
check(label is not None, "no Label parameter on %s" % bd.getScriptName())
label.setValue("<b>a sticky note</b>")  # no font tag
wait_idle()
app.clearSelection()
app.selectNode(bd, True)
NatronGuiTest.sendKey("NodeGraph", "Return")  # open its settings panel: it reads the label
wait_idle()

widgets = QtWidgets.QApplication.allWidgets()
editors = [w for w in widgets if w.objectName() == label.getScriptName()
           and any(isinstance(c, QtWidgets.QFontComboBox) and w.isAncestorOf(c) for c in widgets)]
check(len(editors) == 1, "%d label editors" % len(editors))
bold = [w for w in widgets if isinstance(w, QtWidgets.QPushButton) and editors[0].isAncestorOf(w)
        and "Bold" in w.toolTip()]
check(len(bold) == 1, "%d Bold buttons" % len(bold))
check(bold[0].isChecked(), "Bold is not checked for %r" % label.getValue())
text_edits = [w for w in widgets if isinstance(w, QtWidgets.QTextEdit) and editors[0].isAncestorOf(w)]
check(len(text_edits) == 1, "%d text fields in the label editor" % len(text_edits))

text_edits[0].setPlainText("another note")
check(QtCore.QMetaObject.invokeMethod(text_edits[0], "editingFinished"), "editingFinished() failed")
wait_idle()
value = label.getValue()
check(value.startswith("<b>") and value.endswith("</b>") and "another note" in value,
      "the edited label is %r: not bold" % value)
