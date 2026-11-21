# Regression test for #1554: app.closeProject() run from the Script Editor, with an empty project, quits
# Natron (its only instance). The instance used to quit, and delete its window, inside the call, under
# ScriptEditor::onExecScriptClicked(), which then writes to its output (a use after free once the window
# is deleted when its instance quits). The test passes only if Natron then exits cleanly.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtWidgets
except ImportError:
    try:
        from PySide6 import QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")

for n in app.getChildren():
    n.destroy(False)
wait_idle()
check(not app.getChildren(), "the project still has nodes")

editors = [w for w in QtWidgets.QApplication.allWidgets() if w.metaObject().className() == "Natron::ScriptEditor"]
check(len(editors) == 1, "%d Script Editors" % len(editors))
editor = editors[0]
inputs = [w for w in editor.findChildren(QtWidgets.QWidget) if w.metaObject().className() == "Natron::InputScriptTextEdit"]
check(len(inputs) == 1, "%d Script Editor inputs" % len(inputs))
buttons = [w for w in editor.findChildren(QtWidgets.QAbstractButton) if "Execute the current script" in w.toolTip()]
check(len(buttons) == 1, "%d Script Editor execute buttons" % len(buttons))
inputs[0].setPlainText("app.closeProject()")
buttons[0].click()
print("closeProject() returned from the Script Editor")
