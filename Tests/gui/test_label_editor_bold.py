# The label editor of a backdrop on a bold label without Natron's font tag (e.g. set from Python): the bold
# goes outside the font tag that the editor adds, as the editor writes it, so that the editor shows the
# text without the tags, Italic can be turned on and off, and editing the text keeps the bold. Bold used to
# be checked but dropped when the text was edited (#1807).
import NatronEngine
import NatronGuiTest
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


def click(button):
    """Clicks a button of a label editor."""
    button.click()
    wait_idle()


def edit_text(text_edit, text):
    """Types text in a label editor and leaves the field."""
    text_edit.setPlainText(text)
    check(QtCore.QMetaObject.invokeMethod(text_edit, "editingFinished"), "editingFinished() failed")
    wait_idle()


class Editor(object):
    """The label editor of a rich-text parameter, found in the open settings panels."""

    def __init__(self, param, exclude=()):
        widgets = QtWidgets.QApplication.allWidgets()
        editors = [w for w in widgets if w.objectName() == param.getScriptName()
                   and not any(w is e for e in exclude)
                   and any(isinstance(c, QtWidgets.QFontComboBox) and w.isAncestorOf(c) for c in widgets)]
        check(len(editors) == 1, "%d label editors for %s" % (len(editors), param.getScriptName()))
        self.widget = editors[0]
        self.param = param

        def find(cls, tip):
            found = [w for w in widgets if isinstance(w, cls) and self.widget.isAncestorOf(w)
                     and (tip is None or tip in w.toolTip())]
            check(len(found) == 1, "%d %s in the editor of %s" % (len(found), tip or cls.__name__,
                                                               param.getScriptName()))
            return found[0]

        self.bold = find(QtWidgets.QPushButton, "Bold")
        self.italic = find(QtWidgets.QPushButton, "Italic")
        self.text = find(QtWidgets.QTextEdit, None)
        # the panel's Undo button: in its header, a child of the panel that holds the editor
        undo = [w for w in widgets if isinstance(w, QtWidgets.QPushButton) and "Undo the last change" in w.toolTip()
                and w.parentWidget() is not None and w.parentWidget().parentWidget() is not None
                and w.parentWidget().parentWidget().isAncestorOf(self.widget)]
        check(len(undo) == 1, "%d Undo buttons in the panel of %s" % (len(undo), param.getScriptName()))
        self.undo_button = undo[0]
        # keep the Python object of the panel: when it is freed, PySide invalidates the one of its button,
        # which parentWidget() made its child
        self.panel = self.undo_button.parentWidget().parentWidget()

    def undo(self):
        """Clicks the Undo button of the panel."""
        check(self.undo_button.isEnabled(), "nothing to undo")
        click(self.undo_button)

    def check(self, expected, buttons, what):
        """Checks the parameter value and the (Bold, Italic) checked states."""
        value = self.param.getValue()
        check(value == expected, "%s: the label is %r, not %r" % (what, value, expected))
        state = (self.bold.isChecked(), self.italic.isChecked())
        check(state == buttons, "%s: Bold, Italic checked: %r, not %r" % (what, state, buttons))


ed = Editor(label)
check(ed.widget is editors[0], "another label editor was found")
edit_text(ed.text, "a note")
plain = label.getValue()
check(plain.startswith('<font size="') and plain.endswith("a note</font>"), "the edited label is %r" % plain)
bolded = "<b>" + plain + "</b>"
italicized = "<i>" + plain + "</i>"

# A Python setValue() changes the label behind the Bold and Italic buttons: the buttons follow the label,
# and a click that disagrees with the label only shows its state. The buttons used to keep their state, so
# the next click removed tags that weren't there (in Release, the last characters of the label) or added a
# second <b>.
label.setValue(bolded)
wait_idle()
ed.check(bolded, (True, False), "after setValue() of a bold label")
click(ed.bold)
ed.check(plain, (False, False), "after setValue() of a bold label and Bold")
label.setValue(bolded)
wait_idle()
label.setValue(plain)
wait_idle()
ed.check(plain, (False, False), "after setValue() of a label that isn't bold")
click(ed.bold)
ed.check(bolded, (True, False), "after setValue() of a label that isn't bold and Bold")
label.setValue(italicized)
wait_idle()
ed.check(italicized, (False, True), "after setValue() of an italic label")
click(ed.italic)
ed.check(plain, (False, False), "after setValue() of an italic label and Italic")

# a button out of step with the label (its state set without a click): the click shows the label's state
label.setValue(bolded)
wait_idle()
ed.bold.setChecked(False)
click(ed.bold)
ed.check(bolded, (True, False), "after Bold on a bold label")

# a label without a font tag that only starts with a bold part is not bold as a whole, and the editor shows
# the label's own bold
label.setValue("<b>Note</b>: <b>keep</b>")
wait_idle()
ed.check("<b>Note</b>: <b>keep</b>", (False, False), "a label with two bold parts")
check(ed.text.toPlainText() == "<b>Note</b>: <b>keep</b>", "the editor shows %r" % ed.text.toPlainText())
click(ed.bold)
ed.check("<b><b>Note</b>: <b>keep</b></b>", (True, False), "after Bold on a label with two bold parts")
click(ed.bold)
ed.check("<b>Note</b>: <b>keep</b>", (False, False), "after Bold off a label with two bold parts")

# a label without a font tag, bold around an italic part: the editor removes the bold only, and a text
# edit keeps the italic
label.setValue("<b><i>x</i></b>")
wait_idle()
ed.check("<b><i>x</i></b>", (True, False), "a bold label with an italic part")
check(ed.text.toPlainText() == "<i>x</i>", "the editor shows %r" % ed.text.toPlainText())
edit_text(ed.text, "<i>y</i>")
value = label.getValue()
check(value.startswith('<b><font size="') and value.endswith("<i>y</i></font></b>"),
      "the label edited after a bold label with an italic part is %r" % value)

# a bold part before Natron's font tag is not the editor's bold, and Italic goes right before the font tag
label.setValue("<b>Title</b>" + plain)
wait_idle()
ed.check("<b>Title</b>" + plain, (False, False), "a label with a bold title before the font tag")
click(ed.italic)
ed.check("<b>Title</b><i>" + plain + "</i>", (False, True), "after Italic on a label with a bold title")
label.setValue("<b>Title</b>" + plain + " <b>foot</b>")
wait_idle()
ed.check("<b>Title</b>" + plain + " <b>foot</b>", (False, False), "a label with bold text around the font tag")


def open_panel(node):
    """Opens the settings panel of a node."""
    app.clearSelection()
    app.selectNode(node, True)
    NatronGuiTest.sendKey("NodeGraph", "Return")
    wait_idle()


# "<b>Note: <b>keep</b></b>" when the panel opens is bold as a whole, as when the panel is open
bd2 = app.createNode("fr.inria.built-in.BackDrop")
check(bd2 is not None, "createNode(BackDrop) failed")
label2 = bd2.getParam("Label")
label2.setValue("<b>Note: <b>keep</b></b>")
wait_idle()
open_panel(bd2)
ed2 = Editor(label2, exclude=(ed.widget,))
value = label2.getValue()
check(ed2.bold.isChecked(), "Bold is not checked for %r" % value)
check(value.startswith('<b><font size="') and value.endswith("Note: <b>keep</b></font></b>"),
      "the label of the second backdrop is %r" % value)

# Undo: the backdrop label isn't undoable (it doesn't evaluate on change), a rich-text user parameter is.
# After an undo, the buttons follow the parameter.
bd3 = app.createNode("fr.inria.built-in.BackDrop")
check(bd3 is not None, "createNode(BackDrop) failed")
note = bd3.createStringParam("note", "Note")
check(note is not None, "createStringParam() failed")
note.setType(NatronEngine.StringParam.TypeEnum.eStringTypeRichTextMultiLine)
bd3.refreshUserParamsGUI()
wait_idle()
open_panel(bd3)
ed3 = Editor(note)
edit_text(ed3.text, "undo note")
plain3 = note.getValue()
check(plain3.startswith('<font size="') and plain3.endswith("undo note</font>"), "the edited note is %r" % plain3)
bolded3 = "<b>" + plain3 + "</b>"
italicized3 = "<i>" + plain3 + "</i>"


def start(value):
    """Undoes every edit of the panel, then sets the note from Python (not an undo step).

    Consecutive edits of a parameter are one undo step (KnobUndoCommand::mergeWith()): with an empty undo
    history, the next click is a step of its own, and its undo goes back to value.
    """
    for _ in range(100):
        if not ed3.undo_button.isEnabled():
            break
        ed3.undo()
    check(not ed3.undo_button.isEnabled(), "the undo history of the panel is not empty")
    note.setValue(value)
    wait_idle()


start(plain3)
ed3.check(plain3, (False, False), "a note without bold or italic")
click(ed3.bold)
ed3.check(bolded3, (True, False), "after Bold on")
ed3.undo()
ed3.check(plain3, (False, False), "after Bold on and undo")
click(ed3.bold)
ed3.check(bolded3, (True, False), "after Bold on, undo and Bold")

start(bolded3)
ed3.check(bolded3, (True, False), "a bold note")
click(ed3.bold)
ed3.check(plain3, (False, False), "after Bold off")
ed3.undo()
ed3.check(bolded3, (True, False), "after Bold off and undo")
click(ed3.bold)
ed3.check(plain3, (False, False), "after Bold off, undo and Bold")

start(plain3)
click(ed3.italic)
ed3.check(italicized3, (False, True), "after Italic on")
ed3.undo()
ed3.check(plain3, (False, False), "after Italic on and undo")
click(ed3.italic)
ed3.check(italicized3, (False, True), "after Italic on, undo and Italic")

start(italicized3)
click(ed3.italic)
ed3.check(plain3, (False, False), "after Italic off")
ed3.undo()
ed3.check(italicized3, (False, True), "after Italic off and undo")
click(ed3.italic)
ed3.check(plain3, (False, False), "after Italic off, undo and Italic")

# a text edit after undoing Bold on: the edited note isn't bold
start(plain3)
click(ed3.bold)
ed3.undo()
ed3.check(plain3, (False, False), "after Bold on and undo, before a text edit")
edit_text(ed3.text, "undo note 2")
value = note.getValue()
check(value.startswith('<font size="') and value.endswith("undo note 2</font>"),
      "the note edited after undoing Bold on is %r" % value)
