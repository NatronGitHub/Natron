# Regression test for #1527: "Export as PyPlug" of a converted PyPlug pre-fills the Version field with the
# version of the PyPlug, not with 1 (the version of the Group plug-in), which overwrote an installed v3 PyPlug
# as v1. The PyPlug, Tests/data/pyplugs/NatronTestExportVersion.py, is found through NATRON_PLUGIN_PATH
# (Tests/CMakeLists.txt). The dialog is cancelled: nothing is written.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtWidgets
except ImportError:
    from PySide6 import QtWidgets

PYPLUG = "fr.natron.test.ExportVersion"
require_plugin(PYPLUG)

node = app.createNode(PYPLUG)
check(node is not None, "createNode(%s) failed" % PYPLUG)
wait_idle()
# Export as PyPlug is only offered once the PyPlug is converted to a Group
node.getParam("convertToGroup").trigger()
wait_idle()

versions = []


def readVersion(title, text, className):
    """Record the Version field of the Export as PyPlug dialog, then cancel it."""
    dialog = QtWidgets.QApplication.activeModalWidget()
    if dialog is not None:
        versions.extend(w.text() for w in dialog.findChildren(QtWidgets.QLineEdit)
                        if w.metaObject().className().endswith("SpinBox"))
    return "cancel"


NatronGuiTest.setDialogAnswers([readVersion])
node.getParam("exportAsPyPlug").trigger()
wait_idle()
log = NatronGuiTest.getDialogLog()
print("versions %r, dialogs %r" % (versions, log))
check(len(log) == 1 and log[0].get("answer") == "cancel", "expected one Export as PyPlug dialog: %r" % log)
check(versions == ["3"], "the Version field of the dialog is %r, expected ['3']" % versions)
