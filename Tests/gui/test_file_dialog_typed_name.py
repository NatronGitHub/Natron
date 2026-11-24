# File dialogs: a name typed without a directory is in the directory shown by the dialog, not in Natron's
# working directory (#1534, #1539).
import os
import shutil
import tempfile

import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtGui, QtWidgets


def type_names(*names, fields=None):
    """A dialog answer that types each name in the file name field and presses Return.

    If fields is a list, the text of the field after each Return is appended to it.
    """
    def handler(title, text, class_name):
        dialog = QtWidgets.QApplication.activeModalWidget()
        check(dialog is not None, "no modal file dialog (%s)" % class_name)
        line_edit = dialog.findChild(QtWidgets.QLineEdit, "fileNameLineEdit")
        check(line_edit is not None, "no file name field in the %s" % class_name)
        for name in names:
            line_edit.setText(name)
            press = QtGui.QKeyEvent(QtCore.QEvent.KeyPress, QtCore.Qt.Key_Return, QtCore.Qt.NoModifier)
            QtWidgets.QApplication.sendEvent(line_edit, press)  # the field ignores Return: the dialog gets it
            if fields is not None:
                fields.append(line_edit.text())
        if dialog.isVisible():
            dialog.reject()
        return None
    return handler


def check_path(got, expected, what):
    check(got and os.path.normpath(got) == os.path.normpath(expected), "%s: %r, expected %r" % (what, got, expected))


root = os.path.realpath(tempfile.mkdtemp(prefix="natron-test-file-dialog-"))
old_cwd = os.getcwd()
try:
    shown = os.path.join(root, "shown")
    os.makedirs(os.path.join(shown, "renders"))
    cwd = os.path.join(root, "cwd")
    os.makedirs(os.path.join(cwd, "out"))  # a folder of the working directory only
    os.chdir(cwd)

    # Open dialog: a bare name is in the directory shown (#1534, 1)
    NatronGuiTest.setDialogAnswers([type_names("new_shot.ntp")])
    check_path(app.getFilenameDialog(["ntp"], shown), os.path.join(shown, "new_shot.ntp"), "open, bare name")

    # A subfolder of the directory shown is entered (#1534, 2)
    NatronGuiTest.setDialogAnswers([type_names("renders", "new_shot.ntp")])
    check_path(app.getFilenameDialog(["ntp"], shown), os.path.join(shown, "renders", "new_shot.ntp"),
               "open, subfolder then bare name")

    # Save dialog: a name that is only a folder of the working directory is a file name (#1539, 2)
    NatronGuiTest.setDialogAnswers([type_names("out")])
    check_path(app.saveFilenameDialog(["ntp"], shown), os.path.join(shown, "out.ntp"),
               "save, name of a folder of the working directory")

    # Save dialog: a subfolder of the directory shown is entered, and its name is not kept as the file name
    fields = []
    NatronGuiTest.setDialogAnswers([type_names("renders", fields=fields)])
    app.saveFilenameDialog(["ntp"], shown)  # cancelled by the handler
    check(len(fields) == 1 and os.path.normpath(fields[0]) == os.path.join(shown, "renders"),
          "save, subfolder: the file name field is %r, expected the folder %r" % (fields, os.path.join(shown, "renders")))

    log = NatronGuiTest.getDialogLog()
    check(all(d["type"] == "dialog" for d in log), "dialog log: %r" % log)
finally:
    os.chdir(old_cwd)
    shutil.rmtree(root, ignore_errors=True)
