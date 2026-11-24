# Regression test for #1636: importing a node presets file (.nps) that contains no node shows an error
# instead of crashing (front() on an empty list in NodeSettingsPanel::onImportPresetsActionTriggered()).
import os
import shutil
import tempfile

import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")

require_plugin(GENERATOR)

# a valid presets archive (as written by "Export presets") with NodesCount 0
EMPTY_PRESETS = """<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<!DOCTYPE boost_serialization>
<boost_serialization signature="serialization::archive" version="10">
<NodesCount>0</NodesCount>
</boost_serialization>
"""

tmpdir = tempfile.mkdtemp(prefix="natron-test-presets-")
try:
    presets = os.path.join(tmpdir, "empty.nps")
    with open(presets, "w") as f:
        f.write(EMPTY_PRESETS)

    gen = app.createNode(GENERATOR)
    check(gen is not None, "createNode(%s) failed" % GENERATOR)
    app.clearSelection()
    app.selectNode(gen, True)
    NatronGuiTest.sendKey("NodeGraph", "Return")  # open its settings panel
    wait_idle()
    panels = [w for w in QtWidgets.QApplication.allWidgets()
              if w.metaObject().className() == "Natron::NodeSettingsPanel" and w.objectName() == gen.getScriptName()]
    check(panels, "no settings panel for %s" % gen.getScriptName())

    def open_presets(title, text, className):
        """Answer the file dialog with the presets file, as a user who types its path and presses Open."""
        dialog = QtWidgets.QApplication.activeModalWidget()
        edit = dialog.focusWidget()  # the file name line edit has the focus when the dialog opens
        check(isinstance(edit, QtWidgets.QLineEdit), "%s: the focus widget is %r, not the file name" % (className, edit))
        edit.setText(presets)
        check(QtCore.QMetaObject.invokeMethod(dialog, "openSelectedFiles"), "openSelectedFiles() failed")
        return None

    NatronGuiTest.clearDialogLog()
    NatronGuiTest.setDialogAnswers([open_presets, "ok"])
    # Settings button > Import presets
    check(QtCore.QMetaObject.invokeMethod(panels[0], "onImportPresetsActionTriggered"),
          "onImportPresetsActionTriggered() failed")
    wait_idle()
    log = NatronGuiTest.getDialogLog()
    print("dialogs: %r" % log)
    check(len(log) == 2 and log[0]["answer"] == "handler", "the file dialog was not shown: %r" % log)
    check(log[1]["type"] == "dialog" and log[1]["answer"] == "ok" and presets in log[1]["text"],
          "no error dialog for a presets file without a node: %r" % log)
finally:
    shutil.rmtree(tmpdir, ignore_errors=True)
