# Regression test for #839: the edits of one overlay drag are one undo step, and two drags are two.
# The overlay of net.natron.test.OverlayDrag (Tests/OfxTestPlugin/NatronTestPluginOverlayDrag.cpp) sets its
# parameters y then x in one edit block at each pen motion, as the Transform overlay sets all its parameters,
# but y changes only at every other motion. The first changed parameter of a block makes a new undo command,
# which was merged into the previous one only by a check that depends on heap layout (#1445): the second
# motion of a drag, where y changes first, could start a new undo step, and the next drag could be merged
# into the previous one.
import NatronGuiTest
from natrontest import *

DRAG = "net.natron.test.OverlayDrag"
require_plugin(GENERATOR)
require_plugin(DRAG)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
node = app.createNode(DRAG)
check(node is not None, "createNode(%s) failed" % DRAG)
node.connectInput(0, gen)
viewer.connectInput(0, node)

# the overlay of a node is shown while its settings panel is open: select it and press Return
app.clearSelection()
app.selectNode(node, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
fit_viewer()

x = node.getParam("x")
y = node.getParam("y")
cx, cy = rod_center(gen)


def values():
    return (x.getValue(), y.getValue())


check(values() == (0.0, 0.0), "initial x, y: %r" % (values(),))

NatronGuiTest.drag("Viewer1", cx, cy, cx + 200, cy, steps=10)
wait_idle()
first = values()
print("after the first drag: %r" % (first,))
check_close(first[0], cx + 200, 10.0, "x after the first drag")
check(first[1] > 0, "y after the first drag: %r" % (first[1],))

NatronGuiTest.drag("Viewer1", cx + 200, cy, cx + 300, cy, steps=10)
wait_idle()
second = values()
print("after the second drag: %r" % (second,))
check_close(second[0], cx + 300, 10.0, "x after the second drag")

# one Undo per drag
NatronGuiTest.sendKey("Viewer1", "Ctrl+Z")
wait_idle()
print("after one undo: %r" % (values(),))
check(values() == first, "after one undo, x, y are %r instead of %r (the values after the first drag)" % (values(), first))
NatronGuiTest.sendKey("Viewer1", "Ctrl+Z")
wait_idle()
print("after two undos: %r" % (values(),))
check(values() == (0.0, 0.0), "after two undos, x, y are %r instead of (0, 0)" % (values(),))
