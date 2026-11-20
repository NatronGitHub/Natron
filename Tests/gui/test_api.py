# Public API of #1455 part 2: undo/redo/history, waitForRenders, viewer fit/zoom/centre, getPixel/getImage,
# widget names.
import NatronGuiTest
from natrontest import *

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

# the float image, before the display LUT: the generator fills it with 0.5
x, y = rod_center(gen)
px = v.getPixel(x, y)
check_close(px.r, 0.5, 1e-3, "getPixel() red")
check_close(px.a, 0.5, 1e-3, "getPixel() alpha")
bounds, pixels = v.getImage()
check(len(pixels) > 0 and len(pixels) % 4 == 0, "getImage() pixels")
check_close(pixels[0], 0.5, 1e-3, "getImage() first red")
check(bounds.width() > 0 and bounds.height() > 0, "getImage() bounds")

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
try:
    from PySide2 import QtWidgets
except ImportError:
    from PySide6 import QtWidgets
names = set(w.objectName() for w in QtWidgets.QApplication.allWidgets())
check("nodeGraph" in names, "the node graph is named nodeGraph")
check("Viewer1" in names, "the viewer is named Viewer1")
