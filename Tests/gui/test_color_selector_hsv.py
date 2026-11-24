# Color selector: H, S and V are those of the linear RGB values, also when the color is set through the
# hex field, which sets the sRGB triangle (#1248).
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtWidgets
except ImportError:
    from PySide6 import QtWidgets

PARAMS = "net.natron.test.Params"  # has RGB and RGBA parameters (Tests/OfxTestPlugin)
require_plugin(PARAMS)

# created as the user does (Other > Test > ParamsTest in the node graph menu), so that its settings panel,
# with the parameter widgets, is open (app.createNode() doesn't open it)
viewer = viewer_node(app)
vx, vy = viewer.getPosition()
NatronGuiTest.setDialogAnswers(["ParamsTest"])
NatronGuiTest.penEvent("NodeGraph", "press", vx - 300, vy - 300, button="right")
NatronGuiTest.penEvent("NodeGraph", "release", vx - 300, vy - 300, button="right")
wait_idle()
created = [n for n in app.getChildren() if n.getPluginID() == PARAMS]
check(len(created) == 1, "the node graph menu created %d %s nodes" % (len(created), PARAMS))
node = created[0]


def child(widget, name):
    found = widget.findChild(QtWidgets.QLineEdit, name)
    check(found is not None, "no %s in the color selector" % name)
    return found


hexes = [w for w in QtWidgets.QApplication.allWidgets() if w.objectName() == "ColorSelectorHex"]
check(len(hexes) > 0, "no color selector for %s" % node.getScriptName())
for hex_field in hexes:
    selector = hex_field.parentWidget()
    while selector is not None and selector.findChild(QtWidgets.QLineEdit, "ColorSelectorHue") is None:
        selector = selector.parentWidget()
    check(selector is not None, "no H field next to the hex field")
    # #ff8000 is the sRGB color of the linear (1, 0.2159, 0), whose hue is 0.2159 / 6 = 0.0360 (the hue
    # of the sRGB values, 0.502 / 6 = 0.0837, is what the triangle used to give)
    hex_field.setText("#ff8000")
    hex_field.returnPressed.emit()
    red = float(child(selector, "ColorSelectorRed").text())
    green = float(child(selector, "ColorSelectorGreen").text())
    hue = float(child(selector, "ColorSelectorHue").text())
    sat = float(child(selector, "ColorSelectorSat").text())
    val = float(child(selector, "ColorSelectorVal").text())
    check_close(red, 1.0, 1e-3, "R of #ff8000")
    check_close(green, 0.2159, 2e-3, "G of #ff8000 (linear)")
    check_close(hue, 0.0360, 2e-3, "H of #ff8000 (of the linear values)")
    check_close(sat, 1.0, 1e-3, "S of #ff8000")
    check_close(val, 1.0, 1e-3, "V of #ff8000")
