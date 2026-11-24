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

# "Duplicate Nodes", in the Edit submenu (QMenu::exec() of the node graph menu returns it)
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


def check_not_a_clone(copy, what):
    # A clone's parameters follow the original's (#1512: the menu's Duplicate and Paste used to clone).
    gen.getParam("par").setValue(2.0)
    wait_idle()
    par = copy.getParam("par").getValue()
    gen.getParam("par").setValue(1.0)
    wait_idle()
    check(par == 1.0, "%s made a clone of %s: its par follows the original's (%r)" % (what, gen.getScriptName(), par))


new_gens = [n for n in nodes_of(GENERATOR) if not same_node(n, gen)]
check_not_a_clone(new_gens[0], "Duplicate Nodes")

# "Paste Nodes" pastes the clipboard (a copy, not a clone), also with nothing selected
app.clearSelection()
app.selectNode(gen, True)
app.copySelectedNodes()
app.clearSelection()
known = nodes_of(GENERATOR)
NatronGuiTest.clearDialogLog()
NatronGuiTest.setDialogAnswers(["Paste Nodes"])
right_click("NodeGraph", x + 300, y + 300)  # empty area
log = NatronGuiTest.getDialogLog()
check(len(log) == 1 and log[0]["answer"] == "Paste Nodes", "dialog log: %r" % log)
pasted = [n for n in nodes_of(GENERATOR) if not any(same_node(n, k) for k in known)]
check(len(pasted) == 1, "Paste Nodes did not paste the copied %s: %d new nodes" % (gen.getScriptName(), len(pasted)))
check_not_a_clone(pasted[0], "Paste Nodes")

# an item given by its path: Other > Dot (the submenus are opened on the way, and are not answered again)
DOT = "fr.inria.built-in.Dot"
before = len(nodes_of(DOT))
NatronGuiTest.clearDialogLog()
NatronGuiTest.setDialogAnswers(["Other/Dot"])
right_click("NodeGraph", x - 300, y - 300)  # empty area
log = NatronGuiTest.getDialogLog()
check(len(log) == 1 and log[0]["answer"] == "Other/Dot", "dialog log: %r" % log)
check(len(nodes_of(DOT)) == before + 1, "the Dot item of the Other submenu did not create a Dot")

print("dialogs: %r" % NatronGuiTest.getDialogLog())
