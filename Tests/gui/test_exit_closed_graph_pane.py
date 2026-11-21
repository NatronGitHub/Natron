# Regression test for #1554: Natron exits cleanly after the Node Graph tab was removed from its pane,
# which reparents the node graph to the main window, whose destructor deleted the graph's scene before it.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtWidgets
except ImportError:
    try:
        from PySide6 import QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")

require_plugin(GENERATOR)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
viewer.connectInput(0, gen)
wait_idle()

graphPane = None
graphIndex = -1
for i in range(1, 12):
    p = app.getTabWidget("pane%d" % i)
    if p is not None:
        labels = [p.getTabLabel(j) for j in range(p.count())]
        if "Node Graph" in labels:
            graphPane = p
            graphIndex = labels.index("Node Graph")
check(graphPane is not None, "no pane with the Node Graph")
graphPane.removeTab(graphIndex)
wait_idle()
graphs = [w for w in QtWidgets.QApplication.allWidgets() if w.objectName() == "nodeGraph"]
check(len(graphs) == 1, "%d node graphs" % len(graphs))
parent = graphs[0].parentWidget()
parentClass = parent.metaObject().className() if parent is not None else None
print("node graph parent: %s" % parentClass)
check(parentClass == "Natron::Gui", "the node graph was not reparented to the main window")
