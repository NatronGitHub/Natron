# Regression test for #1445: two edit blocks that change different parameters are two undo steps, whatever
# the order of the parameters in memory. In the viewer, the overlay of net.natron.test.OverlayDrag
# (Tests/OfxTestPlugin/NatronTestPluginOverlayDrag.cpp) adds 1 to x when E is pressed and to y when T is pressed,
# each in its own edit block, outside any drag. MultipleKnobEditsUndoCommand::mergeWith() merged the second block
# into the first one when the second parameter came after the first one in its walk, so that one of the two orders
# below was a single undo step. Two blocks that change the same parameter are still one undo step.
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


def values():
    return (x.getValue(), y.getValue())


def press(key):
    NatronGuiTest.sendKey("Viewer1", key)
    wait_idle()


def undo():
    NatronGuiTest.sendKey("Viewer1", "Ctrl+Z")
    wait_idle()


check(values() == (0.0, 0.0), "initial x, y: %r" % (values(),))

# x then y, then y then x: one undo step per key press in both orders
for keys, after_first in (("ET", (1.0, 0.0)), ("TE", (0.0, 1.0))):
    for key in keys:
        press(key)
    print("after %s: %r" % (" then ".join(keys), values()))
    check(values() == (1.0, 1.0), "after %s, x, y are %r instead of (1, 1)" % (" then ".join(keys), values()))
    undo()
    print("after one undo: %r" % (values(),))
    check(values() == after_first,
          "after %s and one undo, x, y are %r instead of %r: the two edits were one undo step"
          % (" then ".join(keys), values(), after_first))
    undo()
    print("after two undos: %r" % (values(),))
    check(values() == (0.0, 0.0), "after %s and two undos, x, y are %r instead of (0, 0)" % (" then ".join(keys), values()))

# the same parameter twice: one undo step
press("E")
press("E")
print("after E then E: %r" % (values(),))
check(values() == (2.0, 0.0), "after E then E, x, y are %r instead of (2, 0)" % (values(),))
undo()
print("after one undo: %r" % (values(),))
check(values() == (0.0, 0.0),
      "after E then E and one undo, x, y are %r instead of (0, 0): the two edits of x were two undo steps"
      % (values(),))
