# Regression test for #1462: "Group from selection" (Ctrl+G) keeps the links between the selected nodes when a
# selected node feeds several selected nodes; only the inputs coming from outside the selection get Input nodes.
import NatronGuiTest
from natrontest import *

DOT = "fr.inria.built-in.Dot"
GROUP = "fr.inria.built-in.Group"
INPUT = "fr.inria.built-in.Input"


def dot(name, x, y):
    n = app.createNode(DOT)
    check(n is not None, "createNode(Dot) failed")
    n.setScriptName(name)
    n.setPosition(x, y)
    return n


def nodes_of(parent, plugin_id):
    return [n for n in parent.getChildren() if n.getPluginID() == plugin_id]


def input_name(node, i):
    inp = node.getInput(i)
    return None if inp is None else inp.getScriptName()


# src -> A -> B -> out_b, and A -> C -> out_c; A, B and C are selected
src = dot("src", 0, 0)
a = dot("A", 0, 100)
b = dot("B", -100, 200)
c = dot("C", 100, 200)
out_b = dot("out_b", -100, 300)
out_c = dot("out_c", 100, 300)
a.connectInput(0, src)
b.connectInput(0, a)
c.connectInput(0, a)
out_b.connectInput(0, b)
out_c.connectInput(0, c)
wait_idle()

app.setSelection([a, b, c])  # selectNode(n, True) would clear the previous selection
NatronGuiTest.sendKey("NodeGraph", "Ctrl+G")
wait_idle()


def check_grouped(when):
    groups = nodes_of(app, GROUP)
    check(len(groups) == 1, "%s: one group: %r" % (when, [g.getScriptName() for g in groups]))
    group = groups[0]
    names = sorted(n.getScriptName() for n in group.getChildren())
    for name in ("A", "B", "C"):
        check(group.getNode(name) is not None, "%s: no %s in the group: %r" % (when, name, names))
    for name in ("B", "C"):
        inp = input_name(group.getNode(name), 0)
        check(inp == "A", "%s: %s's input in the group: %r" % (when, name, inp))
    inputs = nodes_of(group, INPUT)
    check(len(inputs) == 1, "%s: one Input node (for A's input), got %r" % (when, [n.getScriptName() for n in inputs]))
    check(input_name(group.getNode("A"), 0) == inputs[0].getScriptName(), "%s: A's input in the group: %r" % (when, input_name(group.getNode("A"), 0)))
    check(input_name(group, 0) == "src", "%s: the group's input: %r" % (when, input_name(group, 0)))
    check(all(app.getNode(n) is None for n in ("A", "B", "C")), "%s: the selected nodes are deleted" % when)


def check_ungrouped(when):
    check(len(nodes_of(app, GROUP)) == 0, "%s: the group is deleted" % when)
    for name in ("B", "C"):
        inp = input_name(app.getNode(name), 0)
        check(inp == "A", "%s: %s's input: %r" % (when, name, inp))
    check(input_name(app.getNode("A"), 0) == "src", "%s: A's input: %r" % (when, input_name(app.getNode("A"), 0)))
    check(input_name(out_b, 0) == "B" and input_name(out_c, 0) == "C", "%s: the outputs are connected to B and C" % when)


check_grouped("Group from selection")
app.undo()
wait_idle()
check_ungrouped("undo")
app.redo()
wait_idle()
check_grouped("redo")
app.undo()
wait_idle()
check_ungrouped("undo after redo")
