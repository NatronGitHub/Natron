# Regression test for #1554: Natron crashed at exit when a script had wrapped Natron's widgets with
# PySide (QApplication.topLevelWidgets(), findChildren()) while the settings panel of a node was open.
# The Python wrappers outlive the script: the functions defined here keep its globals alive until Python
# is finalized at exit. The test passes only if Natron then exits cleanly: a crash makes it fail even
# though the script itself passed.
import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtWidgets
    except ImportError:
        NatronGuiTest.skip("PySide2/PySide6 not available")

require_plugin(GENERATOR)


def pump(n=10):
    for i in range(n):
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 20)


viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
viewer.connectInput(0, gen)
wait_idle()

# open the settings panel of the node (Return in the node graph)
app.clearSelection()
app.selectNode(gen, True)
NatronGuiTest.sendKey("NodeGraph", "Return")
wait_idle()

qapp = QtWidgets.QApplication.instance()
check(qapp is not None, "no QApplication")
tl = list(qapp.topLevelWidgets())
visible = [w for w in tl if w.isVisible()]
check(visible, "no visible top-level widget")
mw = max(visible, key=lambda w: w.width() * w.height())
children = mw.findChildren(QtWidgets.QWidget)
panels = [w for w in children if w.metaObject().className().endswith("DockablePanel")
          or w.metaObject().className().endswith("NodeSettingsPanel")]
check(panels, "no settings panel found under %s" % mw.metaObject().className())
print("wrapped %d top-level widgets, %d widgets in %s, %d panels"
      % (len(tl), len(children), mw.metaObject().className(), len(panels)))
pump()
