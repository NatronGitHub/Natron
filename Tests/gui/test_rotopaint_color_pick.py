# Color picking from the viewer while a RotoPaint paint tool is selected (#206): once a color parameter's
# picker is armed, Ctrl+click in the viewer picks the color under the pointer instead of painting a stroke.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtGui, QtWidgets

PARAMS = "net.natron.test.Params"  # has RGB and RGBA parameters (Tests/OfxTestPlugin)
ROTOPAINT = "fr.inria.built-in.RotoPaint"
require_plugin(GENERATOR)
require_plugin(PARAMS)
# the strokes are made of openfx-misc nodes (RotoDrawableItem::createNodes())
require_plugin("net.sf.openfx.MergePlugin")
require_plugin("net.sf.openfx.RotoPlugin")

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
roto = app.createNode(ROTOPAINT)
roto.connectInput(0, gen)
viewer.connectInput(0, roto)

# the overlay of a node is shown while its settings panel is open: select it and press Return
app.clearSelection()
app.selectNode(roto, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
fit_viewer()

# created as the user does (in the node graph menu), so that its settings panel, with the picker of
# its RGB parameter, is open (app.createNode() doesn't open it); nothing is selected, so that it is not
# connected between RotoPaint and the viewer
app.clearSelection()
vx, vy = viewer.getPosition()
NatronGuiTest.setDialogAnswers(["ParamsTest"])
NatronGuiTest.penEvent("NodeGraph", "press", vx - 300, vy - 300, button="right")
NatronGuiTest.penEvent("NodeGraph", "release", vx - 300, vy - 300, button="right")
wait_idle()
created = [n for n in app.getChildren() if n.getPluginID() == PARAMS]
check(len(created) == 1, "the node graph menu created %d %s nodes" % (len(created), PARAMS))
check(same_node(viewer.getInput(0), roto), "the viewer is not connected to RotoPaint")
rgb = created[0].getParam("rgbFrom")
check(rgb is not None, "no rgbFrom parameter")
rgb.setValue(0.0, 0)
rgb.setValue(0.0, 1)
rgb.setValue(0.0, 2)

# a new RotoPaint has the Brush tool selected (pressing N would go to the next tool, the Pencil)


def strokes():
    return len(roto.getRotoContext().getBaseLayer().getChildren())


check(strokes() == 0, "RotoPaint has %d items before the test" % strokes())

# arm the picker of the rgbFrom parameter: click the color swatch next to its fields
pickers = [w for w in QtWidgets.QApplication.allWidgets()
           if w.metaObject().className().endswith("ColorPickerLabel") and w.parentWidget() is not None
           and w.parentWidget().property("paramName") == "rgbFrom"]
check(len(pickers) == 1, "%d color pickers for the rgbFrom parameter" % len(pickers))
picker = pickers[0]
center = QtCore.QPointF(picker.width() / 2.0, picker.height() / 2.0)
for event_type in (QtCore.QEvent.MouseButtonPress, QtCore.QEvent.MouseButtonRelease):
    QtWidgets.QApplication.sendEvent(picker, QtGui.QMouseEvent(event_type, center, QtCore.Qt.LeftButton,
                                                               QtCore.Qt.LeftButton, QtCore.Qt.NoModifier))
wait_idle()

# Ctrl+click on the image (the Generator fills it with 0.5): the color is picked, no stroke is painted
x, y = rod_center(gen)
NatronGuiTest.penEvent("Viewer1", "press", x, y, button="left", modifiers="Ctrl")
NatronGuiTest.penEvent("Viewer1", "release", x, y, button="left", modifiers="Ctrl")
wait_idle()
picked = (rgb.getValue(0), rgb.getValue(1), rgb.getValue(2))
print("picked %r, RotoPaint items: %d" % (picked, strokes()))
for i in range(3):
    check_close(picked[i], 0.5, 0.01, "picked color [%d]" % i)
check(strokes() == 0, "Ctrl+click painted %d strokes instead of picking the color" % strokes())

# as with the other tools, the next click turns the picker off instead of painting
NatronGuiTest.penEvent("Viewer1", "press", x + 50, y, button="left")
NatronGuiTest.penEvent("Viewer1", "release", x + 50, y, button="left")
wait_idle()
check(strokes() == 0, "the click that turns the picker off painted %d strokes" % strokes())
NatronGuiTest.penEvent("Viewer1", "press", x + 50, y, button="left")
NatronGuiTest.penEvent("Viewer1", "release", x + 50, y, button="left")
wait_idle()
check(strokes() == 1, "with the picker off, a click paints a stroke (%d)" % strokes())
# the Brush was the tool: a Pencil click would have made a "Pencil" item
item = roto.getRotoContext().getBaseLayer().getChildren()[0]
check(item.getScriptName().startswith("Brush"), "the click made %s, not a brush stroke" % item.getScriptName())
