"""A file parameter refuses a path that uses [Project] before the project is saved (#1539).

Until the project is saved, [Project] has no value, so "[Project]/out.exr" would be "/out.exr". Typed in
the parameter, or chosen in its file dialog, such a path is refused: an error says why and the value is
kept. Once the project is saved, the same path is accepted.
"""
import os
import shutil
import tempfile

import NatronGuiTest
from natrontest import *

QtCore, QtGui, QtWidgets = qt_modules()

PATH = "[Project]/out.exr"


def find_line_edit(param_name):
    """The text field of a file parameter in the open settings panels, by its paramName property."""
    edits = [w for w in QtWidgets.QApplication.allWidgets()
             if w.property("paramName") == param_name and isinstance(w, QtWidgets.QLineEdit)]
    check(len(edits) == 1, "%d text fields for %s" % (len(edits), param_name))
    return edits[0]


def press_return(widget):
    """Send Return to the widget, without the keyboard focus (NATRON_TEST_NO_FOCUS)."""
    for event_type in (QtCore.QEvent.KeyPress, QtCore.QEvent.KeyRelease):
        QtWidgets.QApplication.sendEvent(widget, QtGui.QKeyEvent(event_type, QtCore.Qt.Key_Return, QtCore.Qt.NoModifier))


def type_in_dialog(name, still_open):
    """A file dialog answer: types name in the file name field, presses Return, and records whether the
    dialog is still open (it is, if it refused the name), then cancels it."""
    def handler(title, text, class_name):
        dialog = QtWidgets.QApplication.activeModalWidget()
        check(dialog is not None, "no modal file dialog (%s)" % class_name)
        line_edit = dialog.findChild(QtWidgets.QLineEdit, "fileNameLineEdit")
        check(line_edit is not None, "no file name field in the %s" % class_name)
        line_edit.setText(name)
        press_return(line_edit)  # the field ignores Return: the dialog gets it
        still_open.append(dialog.isVisible())
        if dialog.isVisible():
            dialog.reject()
        return None
    return handler


def check_refusals(what, count):
    """Checks that the dialog log holds count refusals of an unset [Project] path, and nothing unexpected."""
    log = NatronGuiTest.getDialogLog()
    refusals = [d for d in log if d["type"] == "dialog" and "until the project is saved" in d["text"]]
    check(len(refusals) == count, "%s: %d refusals, expected %d: %r" % (what, len(refusals), count, log))
    check(all(d["type"] == "dialog" for d in log), "%s: dialog log %r" % (what, log))


folder = os.path.realpath(tempfile.mkdtemp(prefix="natron-test-unsaved-project-path-"))
try:
    g = app.createNode("fr.inria.built-in.Group")
    check(g is not None, "createNode(Group) failed")
    params = (("natronTestIn", g.createFileParam("natronTestIn", "In")),
              ("natronTestOut", g.createOutputFileParam("natronTestOut", "Out")))
    g.refreshUserParamsGUI()
    open_settings_panel(g)

    NatronGuiTest.clearDialogLog()
    for name, param in params:
        # typed in the parameter: refused, and the field shows the value again
        edit = find_line_edit(name)
        NatronGuiTest.setDialogAnswers(["ok"])
        edit.setText(PATH)
        press_return(edit)
        wait_idle()
        check(param.getValue() == "", "%s: typed %r was set: %r" % (name, PATH, param.getValue()))
        check(edit.text() == "", "%s: the field shows %r" % (name, edit.text()))

        # chosen in the file dialog: refused, and the dialog stays open
        still_open = []
        NatronGuiTest.setDialogAnswers([type_in_dialog(PATH, still_open), "ok"])
        param.openFile()
        wait_idle()
        check(still_open == [True], "%s: the file dialog accepted %r" % (name, PATH))
        check(param.getValue() == "", "%s: chosen %r was set: %r" % (name, PATH, param.getValue()))
    check_refusals("unsaved project", 2 * len(params))

    # once the project is saved, [Project] is its folder: the typed path is accepted
    check(app.saveProject(os.path.join(folder, "project.ntp")), "saveProject() failed")
    NatronGuiTest.clearDialogLog()
    for name, param in params:
        edit = find_line_edit(name)
        edit.setText(PATH)
        press_return(edit)
        wait_idle()
        check(param.getValue() == PATH, "%s: saved project, typed %r gave %r" % (name, PATH, param.getValue()))
    check_refusals("saved project", 0)
finally:
    shutil.rmtree(folder, ignore_errors=True)
