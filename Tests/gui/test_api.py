# Public API of #1455 part 2: undo/redo/history, waitForRenders, viewer fit/zoom/centre, getPixel/getImage,
# widget names.
import array

import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtWidgets
except ImportError:
    from PySide6 import QtWidgets

require_plugin(GENERATOR)

viewer = viewer_node(app)
v = app.getViewer("Viewer1")
gen = app.createNode(GENERATOR)
viewer.connectInput(0, gen)
v.fit()
check(app.waitForRenders(30), "waitForRenders()")
check(not NatronGuiTest.isRendering(), "no render after waitForRenders()")

# zoom and centre
z = v.getZoom()
v.setZoom(z * 2)
check_close(v.getZoom(), z * 2, 1e-6, "setZoom()")
c = v.getCenter()
v.setCenter(c.x + 100, c.y + 50)
c2 = v.getCenter()
check_close(c2.x, c.x + 100, 1e-6, "setCenter() x")
check_close(c2.y, c.y + 50, 1e-6, "setCenter() y")
w = v.canvasToWidget(100., 200.)
p = v.widgetToCanvas(w.x, w.y)
check_close(p.x, 100., 1e-6, "widgetToCanvas(canvasToWidget()) x")
check_close(p.y, 200., 1e-6, "widgetToCanvas(canvasToWidget()) y")
v.fit()
check(app.waitForRenders(30), "waitForRenders() after fit()")

# after fit(), the image is centred in the viewer: its centre maps to the centre of the widget
gl = [w for w in QtWidgets.QApplication.allWidgets() if w.objectName() == "Viewer1" and w.metaObject().className().endswith("ViewerGL")]
check(len(gl) == 1, "one ViewerGL named Viewer1")
x, y = rod_center(gen)
cx, cy = v.getCenter().x, v.getCenter().y
check_close(cx, x, 1e-3, "fit() centres the image (x)")
check_close(cy, y, 1e-3, "fit() centres the image (y)")
wc = v.canvasToWidget(x, y)
check_close(wc.x, gl[0].width() / 2.0, 1.0, "canvasToWidget(image centre) x")
check_close(wc.y, gl[0].height() / 2.0, 1.0, "canvasToWidget(image centre) y")

# the float image, before the display LUT: the generator fills it with 0.5
x, y = rod_center(gen)
px = v.getPixel(x, y)
check_close(px.r, 0.5, 1e-3, "getPixel() red")
check_close(px.a, 0.5, 1e-3, "getPixel() alpha")
bounds, data = v.getImage()
pixels = array.array('f', data)
check(len(pixels) > 0 and len(pixels) % 4 == 0, "getImage() pixels")
check_close(pixels[0], 0.5, 1e-3, "getImage() first red")
check(bounds.width() > 0 and bounds.height() > 0, "getImage() bounds")
import NatronEngine
region = NatronEngine.RectD()
region.set(x - 10, y - 10, x + 10, y + 10)
rbounds, rdata = v.getImage(0, region)
check(len(rdata) < len(data) and rbounds.width() <= 21 * 2 ** 3, "getImage() with a region")

# undo/redo/history with the node graph's delete
app.clearSelection()
app.selectNode(gen, True)
NatronGuiTest.sendKey("NodeGraph", "Backspace")
wait_idle()
check(app.canUndo(), "canUndo() after a delete")
n = app.getUndoIndex()
hist = app.getUndoHistory()
check(n == len(hist) and n > 0, "getUndoIndex() == len(getUndoHistory()): %r %r" % (n, hist))
app.undo()
wait_idle()
check(app.getNode(gen.getScriptName()) is not None, "app.undo() restored the node")
check(app.getUndoIndex() == n - 1 and app.canRedo(), "undo index and canRedo()")
app.redo()
wait_idle()
check(app.getNode(gen.getScriptName()) is None, "app.redo() deleted the node again")

# widget names
names = set(w.objectName() for w in QtWidgets.QApplication.allWidgets())
for n in ("nodeGraph", "Viewer1", "TimeLine", "PropertiesBin"):
    check(n in names, "a widget is named %s" % n)
