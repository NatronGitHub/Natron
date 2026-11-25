# Color selector: H, S and V are those of the linear RGB values, also when the color is set through the
# hex field, which sets the sRGB triangle (#1248). A grey has no hue: it keeps the H shown, whether it is
# set through the triangle, the RGB fields or the parameter (the selector shows the parameter when it opens).
import NatronEngine
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtWidgets

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


def type_value(selector, name, value):
    field = child(selector, name)
    field.setText("%g" % value)
    field.returnPressed.emit()


def set_color_params_grey(grey):
    for param in node.getParams():
        if isinstance(param, NatronEngine.ColorParam):
            for dim in range(param.getNumDimensions()):
                param.setValue(1.0 if dim == 3 else grey, dim)


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

    # a grey typed in the RGB fields keeps the hue: (0.2159, 0.2159, 0) is yellow (H = 1/6), then B = 0.2159
    type_value(selector, "ColorSelectorRed", 0.2159)
    check_close(float(child(selector, "ColorSelectorHue").text()), 1. / 6, 2e-3, "H of (0.2159, 0.2159, 0)")
    type_value(selector, "ColorSelectorBlue", 0.2159)
    check_close(float(child(selector, "ColorSelectorSat").text()), 0.0, 1e-3, "S of a grey")
    check_close(float(child(selector, "ColorSelectorHue").text()), 1. / 6, 2e-3, "H of a grey typed in RGB")

    # a grey parameter shown when the selector opens keeps the hue: #00ff00 is green (H = 1/3)
    hex_field.setText("#00ff00")
    hex_field.returnPressed.emit()
    check_close(float(child(selector, "ColorSelectorHue").text()), 1. / 3, 2e-3, "H of #00ff00")
    set_color_params_grey(0.5)
    widget = selector
    while widget is not None and not widget.metaObject().className().endswith("ColorSelectorWidget"):
        widget = widget.parentWidget()
    check(widget is not None, "no ColorSelectorWidget around the hex field")
    QtWidgets.QApplication.sendEvent(widget, QtCore.QEvent(QtCore.QEvent.Show))  # as when its menu opens
    check_close(float(child(selector, "ColorSelectorRed").text()), 0.5, 1e-3, "R of the grey parameter")
    check_close(float(child(selector, "ColorSelectorHue").text()), 1. / 3, 2e-3, "H of a grey parameter")

    # a grey typed in the hex field keeps the hue too (the triangle path)
    hex_field.setText("#808080")
    hex_field.returnPressed.emit()
    check_close(float(child(selector, "ColorSelectorSat").text()), 0.0, 1e-3, "S of #808080")
    check_close(float(child(selector, "ColorSelectorHue").text()), 1. / 3, 2e-3, "H of #808080")
