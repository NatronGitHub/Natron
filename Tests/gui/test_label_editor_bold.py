# The label editor of a backdrop on a bold label without Natron's font tag (e.g. set from Python): the bold
# goes outside the font tag that the editor adds, as the editor writes it, so that the editor shows the
# text without the tags, Italic can be turned on and off, and editing the text keeps the bold. Bold used to
# be checked but dropped when the text was edited (#1807).
from natrontest import *

QtCore, QtGui, QtWidgets = qt_modules()

bd = app.createNode("fr.inria.built-in.BackDrop")
check(bd is not None, "createNode(BackDrop) failed")
label = bd.getParam("Label")
check(label is not None, "no Label parameter on %s" % bd.getScriptName())
label.setValue("<b>a sticky note</b>")  # no font tag
wait_idle()
open_settings_panel(bd)  # it reads the label

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
italic = [w for w in widgets if isinstance(w, QtWidgets.QPushButton) and editors[0].isAncestorOf(w)
          and "Italic" in w.toolTip()]
check(len(italic) == 1, "%d Italic buttons" % len(italic))


def check_wrapped(prefix, suffix, what):
    value = label.getValue()
    check(value.startswith(prefix + '<font size="') and value.endswith("a sticky note</font>" + suffix),
          "%s: the label is %r" % (what, value))


check_wrapped("<b>", "</b>", "after opening the editor")
check(text_edits[0].toPlainText() == "a sticky note", "the editor shows %r" % text_edits[0].toPlainText())

italic[0].click()
wait_idle()
check_wrapped("<i><b>", "</b></i>", "after Italic on")
italic[0].click()
wait_idle()
check_wrapped("<b>", "</b>", "after Italic off")

text_edits[0].setPlainText("another note")
check(QtCore.QMetaObject.invokeMethod(text_edits[0], "editingFinished"), "editingFinished() failed")
wait_idle()
value = label.getValue()
check(value.startswith("<b><font") and value.endswith("another note</font></b>"),
      "the edited label is %r: not bold" % value)

# a label with a bold part, but not bold as a whole (e.g. set from Python while the panel is open): Bold on
# and off adds and removes the editor's bold, not the label's own
bold[0].click()  # Bold off on the edited label
wait_idle()
check(not bold[0].isChecked(), "Bold is still checked")
label.setValue("Note: <b>keep</b>")
wait_idle()
bold[0].click()
wait_idle()
check(label.getValue() == "<b>Note: <b>keep</b></b>", "after Bold on: the label is %r" % label.getValue())
bold[0].click()
wait_idle()
check(label.getValue() == "Note: <b>keep</b>", "after Bold off: the label is %r" % label.getValue())
