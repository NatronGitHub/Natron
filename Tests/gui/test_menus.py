# Menu answers (NatronGuiTest.setDialogAnswers): the node graph context menu uses the action that
# QMenu::exec() returns, so the answer must select the item as the user does, also in a submenu.
import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)


def node_center(node):
    x, y = node.getPosition()
    w, h = node.getSize()
    return (x + w / 2.0, y + h / 2.0)


def right_click(target, x, y):
    NatronGuiTest.penEvent(target, "move", x, y, button="none")
    NatronGuiTest.penEvent(target, "press", x, y, button="right")  # the menu runs in the press
    NatronGuiTest.penEvent(target, "release", x, y, button="right")
    wait_idle()


def nodes_of(pluginID):
    return [n for n in app.getChildren() if n.getPluginID() == pluginID]


gen = app.createNode(GENERATOR)
check(gen is not None, "createNode(%s) failed" % GENERATOR)
wait_idle()

# a top-level item of the menu: "Duplicate Nodes" (QMenu::exec() returns it)
app.clearSelection()
app.selectNode(gen, True)
before = len(nodes_of(GENERATOR))
NatronGuiTest.clearDialogLog()
NatronGuiTest.setDialogAnswers(["Duplicate Nodes"])
x, y = node_center(gen)
right_click("NodeGraph", x, y)
log = NatronGuiTest.getDialogLog()
check(len(log) == 1 and log[0]["type"] == "menu" and log[0]["answer"] == "Duplicate Nodes",
      "dialog log: %r" % log)
check(len(nodes_of(GENERATOR)) == before + 1,
      "Duplicate Nodes did not duplicate %s: %d nodes" % (gen.getScriptName(), len(nodes_of(GENERATOR))))

# an item in a submenu: Other > Dot (the submenu is opened on the way, and is not answered again)
DOT = "fr.inria.built-in.Dot"
before = len(nodes_of(DOT))
NatronGuiTest.clearDialogLog()
NatronGuiTest.setDialogAnswers(["Dot"])
right_click("NodeGraph", x - 300, y - 300)  # empty area
log = NatronGuiTest.getDialogLog()
check(len(log) == 1 and log[0]["answer"] == "Dot", "dialog log: %r" % log)
check(len(nodes_of(DOT)) == before + 1, "the Dot item of the Other submenu did not create a Dot")

print("dialogs: %r" % NatronGuiTest.getDialogLog())
