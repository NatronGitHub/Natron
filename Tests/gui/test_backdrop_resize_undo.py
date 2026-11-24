# Regression test for #1744: undoing the resize of a backdrop restored a slightly larger backdrop
# (ResizeBackdropCommand recorded the old size from boundingRect(), which includes the pen width).
import NatronGuiTest
from natrontest import *

bd = app.createNode("fr.inria.built-in.BackDrop")
check(bd is not None, "createNode(BackDrop) failed")
bd.setSize(400, 300)
wait_idle()
x, y = bd.getPosition()
w, h = bd.getSize()
print("backdrop at (%g, %g), size %g x %g" % (x, y, w, h))

# drag the resize handle (the triangle in the bottom-right corner) by (60, 40)
x0, y0 = x + w - 3, y + h - 3
NatronGuiTest.penEvent("NodeGraph", "move", x0, y0, button="none")
NatronGuiTest.penEvent("NodeGraph", "press", x0, y0, button="left")
for i in range(1, 11):
    NatronGuiTest.penEvent("NodeGraph", "move", x0 + 6 * i, y0 + 4 * i)
NatronGuiTest.penEvent("NodeGraph", "release", x0 + 60, y0 + 40, button="left")
wait_idle()
resized = bd.getSize()
print("resized: %g x %g" % resized)
check(resized[0] > w + 30 and resized[1] > h + 20, "the drag did not resize the backdrop: %r" % (resized,))

app.undo()
wait_idle()
undone = bd.getSize()
check(undone == (w, h), "undo restored %r, not the size before the resize %r" % (undone, (w, h)))
