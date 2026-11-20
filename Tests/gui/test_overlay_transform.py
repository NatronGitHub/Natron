# Overlay interaction: drag the centre handle of the Transform overlay (openfx-misc) in the viewer,
# then undo it from the viewer.
import NatronGuiTest
from natrontest import *

TRANSFORM = "net.sf.openfx.TransformPlugin"
require_plugin(GENERATOR)
require_plugin(TRANSFORM)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
xf = app.createNode(TRANSFORM)
xf.connectInput(0, gen)
viewer.connectInput(0, xf)

# the overlay of a node is shown while its settings panel is open: select it and press Return
app.clearSelection()
app.selectNode(xf, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
fit_viewer()

translate = xf.getParam("translate")
center = xf.getParam("center")
cx, cy = center.getValue(0), center.getValue(1)
print("Transform center %r, translate %r" % ((cx, cy), (translate.getValue(0), translate.getValue(1))))
check_close(translate.getValue(0), 0.0, 1e-6, "initial translate x")

# dragging the centre point moves the image: translate follows the pointer
NatronGuiTest.drag("Viewer1", cx, cy, cx + 200, cy + 100, steps=20)
wait_idle()
tx, ty = translate.getValue(0), translate.getValue(1)
print("translate after the drag: %r" % ((tx, ty),))
check_close(tx, 200.0, 10.0, "translate x after the drag")
check_close(ty, 100.0, 10.0, "translate y after the drag")
check_close(center.getValue(0), cx, 1e-6, "center x after the drag")

# the drag is one undoable change
NatronGuiTest.sendKey("Viewer1", "Ctrl+Z")
wait_idle()
check_close(translate.getValue(0), 0.0, 1e-6, "translate x after undo")
check_close(translate.getValue(1), 0.0, 1e-6, "translate y after undo")
