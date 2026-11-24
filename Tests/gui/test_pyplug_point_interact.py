# Regression test for #1526: Export as PyPlug saves "Use Point Interact" of a 2D parameter, so the point of the
# exported PyPlug can be dragged in the viewer. A Group with such a parameter is exported through the Export as
# PyPlug dialog to a temporary directory, the exported createInstance() fills a second Group (as Natron does
# for a PyPlug), and dragging the point in the viewer must change the parameter of that Group.
import importlib
import os
import sys
import tempfile

import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtWidgets
except ImportError:
    from PySide6 import QtWidgets

GROUP = "fr.inria.built-in.Group"
MODULE = "NatronTestPointInteract"
require_plugin(GENERATOR)

source = app.createNode(GROUP)
check(source is not None, "createNode(%s) failed" % GROUP)
point = source.createDouble2DParam("point", "Point")
point.setUsePointInteract(True)
point.set(500.0, 300.0)
source.refreshUserParamsGUI()
wait_idle()

out_dir = tempfile.mkdtemp(prefix="natron-pyplug-point-interact-")


def fill_dialog(_title, _text, _class_name):
    """Fill the ID, label and directory of the Export as PyPlug dialog (its QGridLayout), and accept it."""
    dialog = QtWidgets.QApplication.activeModalWidget()  # a variable: a temporary wrapper would delete the layout wrapper
    layout = dialog.layout()
    layout.itemAtPosition(0, 1).widget().setText("fr.natron.test.PointInteract")  # ID
    layout.itemAtPosition(1, 1).widget().setText(MODULE)  # label: the file name
    layout.itemAtPosition(6, 1).widget().setText(out_dir)  # directory
    return "ok"


# then "Directory ... is not in the group plug-in search path, would you like to add it?"
NatronGuiTest.setDialogAnswers([fill_dialog, "no"])
source.getParam("exportAsPyPlug").trigger()
wait_idle()
log = NatronGuiTest.getDialogLog()
print("dialogs %r" % (log,))
path = os.path.join(out_dir, MODULE + ".py")
check(os.path.exists(path), "the PyPlug was not exported to %s" % path)
with open(path) as f:
    print("".join(line for line in f if "point" in line.lower()))

# instantiate the exported PyPlug in an empty Group, as Natron does
sys.path.insert(0, out_dir)
sys.dont_write_bytecode = True
module = importlib.import_module(MODULE)
target = app.createNode(GROUP)
for child in target.getChildren():
    child.destroy()
module.createInstance(app, target)
wait_idle()
target_point = target.getParam("point")
check(target_point is not None, "the exported PyPlug has no point parameter")
check_close(target_point.getValue(0), 500.0, 1e-6, "exported point x")

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
target.connectInput(0, gen)
viewer.connectInput(0, target)
source.destroy()  # its overlay is at the same place
# the overlay of a node is shown while its settings panel is open: select it and press Return
app.clearSelection()
app.selectNode(target, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
fit_viewer()

NatronGuiTest.drag("Viewer1", 500.0, 300.0, 700.0, 400.0, steps=20)
wait_idle()
x, y = target_point.getValue(0), target_point.getValue(1)
print("point after the drag: %r" % ((x, y),))
check_close(x, 700.0, 10.0, "point x after the drag")
check_close(y, 400.0, 10.0, "point y after the drag")
