"""Renaming a group recompiles the expressions of its knobs and of the nodes inside it (#1686).

The expressions were compiled with the group's old name, and those that read no parameter of the
renamed nodes (e.g. "thisGroup.getInput(1) is None" in the Glow PyPlug) were not recompiled, so they
failed with "AttributeError: ... has no attribute 'Group1'".
"""

import natronheadless


def main(app):
    group = app.createNode("fr.inria.built-in.Group")
    assert group is not None
    group.setScriptName("Outer")
    inner = app.createNode("fr.inria.built-in.Group", -1, group)
    assert inner is not None
    inner.setScriptName("Inner")
    assert app.createNode("fr.inria.built-in.Input", -1, group) is not None

    # the expressions read no parameter, so the renamed nodes have no listener
    own = group.createIntParam("own", "Own")
    own.setExpression("len(thisNode.getScriptName())", False, 0)
    child = inner.createIntParam("child", "Child")
    child.setExpression("len(thisGroup.getScriptName())", False, 0)
    # as Merge4's disableNode in the Glow PyPlug
    no_input = inner.createBooleanParam("noInput", "No Input")
    no_input.setExpression("thisGroup.getInput(0) is None", False, 0)
    # a listener of a parameter of the renamed node, recompiled by the code that was there before
    listener = inner.createIntParam("listener", "Listener")
    listener.setExpression("thisGroup.own.get()", False, 0)
    assert own.getValueAtTime(1) == len("Outer"), own.getValueAtTime(1)
    assert child.getValueAtTime(1) == len("Outer"), child.getValueAtTime(1)
    assert no_input.getValueAtTime(1)

    group.setScriptName("RenamedOuter")
    assert app.getNode("RenamedOuter.Inner") is not None

    # a frame that was never evaluated, so that the expressions run again
    for param in (own, child, listener):
        value = param.getValueAtTime(2)
        assert value == len("RenamedOuter"), "%s = %s after the rename" % (param.getScriptName(), value)
    assert no_input.getValueAtTime(2), "noInput is False after the rename"


natronheadless.run(main)
