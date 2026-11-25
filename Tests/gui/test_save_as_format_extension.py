# Save As: the file gets the extension of the format chosen in the dialog. The dialog adds the extension of
# its file type menu, which is that of the default format; choosing the other kind of format then used to
# give "comp.py.ntp" (Python default, Binary chosen), and the dialog asked about replacing "comp.py", not
# the file written (#1672).
import os
import shutil
import tempfile

import NatronEngine
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtGui, QtWidgets

FORMATS = NatronEngine.Natron.ProjectFileFormatEnum
BINARY, PYTHON = "Binary", "Python script"  # entries of the "File format" menu of the dialog


def choose_format_and_type(name):
    """A Save As dialog answer: open the format menu (the next answer chooses its entry), type name, Return."""
    def handler(title, text, class_name):
        dialog = QtWidgets.QApplication.activeModalWidget()
        check(dialog is not None, "no modal Save As dialog (%s)" % class_name)
        combo = dialog.findChild(QtWidgets.QWidget, "projectFileFormat")
        check(combo is not None, "no file format menu in the Save As dialog")
        NatronGuiTest.penEvent("projectFileFormat", "press", 5, 5)  # the menu runs in the press
        NatronGuiTest.penEvent("projectFileFormat", "release", 5, 5)
        line_edit = dialog.findChild(QtWidgets.QLineEdit, "fileNameLineEdit")
        check(line_edit is not None, "no file name field in the Save As dialog")
        line_edit.setText(name)
        press = QtGui.QKeyEvent(QtCore.QEvent.KeyPress, QtCore.Qt.Key_Return, QtCore.Qt.NoModifier)
        QtWidgets.QApplication.sendEvent(line_edit, press)  # the field ignores Return: the dialog gets it
        if dialog.isVisible():
            dialog.reject()
        return None
    return handler


def files(directory):
    """The files of directory, without the lock files of the open project."""
    return sorted(f for f in os.listdir(directory) if not f.endswith(".lock"))


root = os.path.realpath(tempfile.mkdtemp(prefix="natron-test-save-as-"))
try:
    existing = os.path.join(root, "comp.ntp")
    with open(existing, "w") as f:
        f.write("not a project")

    # Python default, Binary chosen: "comp" -> "comp.py" (by the dialog) -> "comp.ntp", which exists: the
    # replace question is about it, and "no" keeps it
    NatronGuiTest.clearDialogLog()
    NatronGuiTest.setDialogAnswers([choose_format_and_type(os.path.join(root, "comp")), BINARY, "no"])
    check(not app.saveProject("", FORMATS.eProjectFileFormatPython), "Save As saved although the replace was declined")
    log = NatronGuiTest.getDialogLog()
    check(len(log) == 3 and "comp.ntp" in log[-1].get("text", "") and log[-1].get("answer") == "no",
          "expected the question about replacing comp.ntp: %r" % log)
    with open(existing) as f:
        check(f.read() == "not a project", "comp.ntp was overwritten")
    check(files(root) == ["comp.ntp"], "files after the declined save: %r" % files(root))

    # the same with a new name: "shot.ntp" is written as a binary project
    NatronGuiTest.clearDialogLog()
    NatronGuiTest.setDialogAnswers([choose_format_and_type(os.path.join(root, "shot")), BINARY])
    check(app.saveProject("", FORMATS.eProjectFileFormatPython), "Save As shot failed")
    check(files(root) == ["comp.ntp", "shot.ntp"], "files after Save As shot (Binary): %r" % files(root))
    check(app.getProjectFormat() == FORMATS.eProjectFileFormatBinary, "format %r" % app.getProjectFormat())
    check(len(NatronGuiTest.getDialogLog()) == 2, "dialogs: %r" % NatronGuiTest.getDialogLog())

    # the other way round (Binary default, Python chosen): "take" -> "take.ntp" -> "take.py"
    gui = [w for w in QtWidgets.QApplication.topLevelWidgets() if w.metaObject().className().endswith("Gui")]
    check(len(gui) == 1, "main windows: %r" % gui)
    NatronGuiTest.setDialogAnswers([choose_format_and_type(os.path.join(root, "take")), PYTHON])
    check(QtCore.QMetaObject.invokeMethod(gui[0], "saveProjectAs"), "saveProjectAs() failed")
    wait_idle()
    check(files(root) == ["comp.ntp", "shot.ntp", "take.py"], "files after Save As take (Python): %r" % files(root))
    check(app.getProjectFormat() == FORMATS.eProjectFileFormatPython, "format %r" % app.getProjectFormat())
finally:
    shutil.rmtree(root, ignore_errors=True)
