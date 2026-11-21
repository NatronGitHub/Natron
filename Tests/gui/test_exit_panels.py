# Regression test for #1554: Natron exits cleanly with a Python panel in a floating pane, a floating node
# settings panel, a modal dialog that was created but not shown, and two viewers in one pane.
import NatronGuiTest
import NatronGui
from natrontest import *

try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")

require_plugin(GENERATOR)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
viewer.connectInput(0, gen)
viewer2 = app.createNode("fr.inria.built-in.Viewer")
check(viewer2 is not None, "no second viewer")
viewer2.connectInput(0, gen)
wait_idle()
viewerTabs = [w for w in QtWidgets.QApplication.allWidgets() if w.metaObject().className() == "Natron::ViewerTab"]
check(len(viewerTabs) == 2, "%d viewer tabs" % len(viewerTabs))
check(viewerTabs[0].parentWidget() is not None and viewerTabs[1].parentWidget() is not None, "a viewer tab has no parent")

# a Python panel, registered, in a pane that is then floated
panel = NatronGui.PyPanel("fr.natron.test.exitPanel", "Exit test panel", True, app)
panel.createStringParam("text", "Text")
app.registerPythonPanel(panel, "test_exit_panels_panel")
panes = [p for p in (app.getTabWidget("pane%d" % i) for i in range(1, 12)) if p is not None]
check(panes, "no pane")
pane = panes[-1]
check(pane.appendTab(panel), "appendTab() failed")
pane.floatPane()
wait_idle()

# a modal dialog, created but not shown
dialog = app.createModalDialog()
dialog.addWidget(QtWidgets.QLabel("exit test"))

# the settings panel of the Generator, floated
app.clearSelection()
app.selectNode(gen, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
wait_idle()
panels = [w for w in QtWidgets.QApplication.allWidgets()
          if w.metaObject().className() in ("Natron::NodeSettingsPanel", "Natron::DockablePanel") and w.isVisible()
          and w.objectName() == gen.getScriptName()]
check(panels, "no visible settings panel for %s" % gen.getScriptName())
check(QtCore.QMetaObject.invokeMethod(panels[0], "floatPanel"), "floatPanel() failed")
wait_idle()
floating = [w for w in QtWidgets.QApplication.topLevelWidgets()
            if w.metaObject().className() == "Natron::FloatingWidget" and w.isVisible()]
check(len(floating) >= 2, "%d floating windows" % len(floating))
print("%d floating windows, %d viewer tabs" % (len(floating), len(viewerTabs)))
