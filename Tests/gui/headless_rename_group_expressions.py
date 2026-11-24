# Renaming a group keeps the expressions of its knobs and of the knobs of the nodes inside it working
# (#1686): run by NatronRenderer -t (see Tests/CMakeLists.txt). The expressions were compiled with the
# group's old name, and those that read no parameter of the renamed nodes (e.g. "thisGroup.getInput(1) is
# None" in the Glow PyPlug) were not recompiled, so they failed with "AttributeError: ... has no attribute
# 'Group1'".
from NatronEngine import natron

status = 1
try:
    app = natron.getInstance(0)
    group = app.createNode("fr.inria.built-in.Group")
    assert group is not None
    group.setScriptName("Outer")
    inner = app.createNode("fr.inria.built-in.Group", -1, group)
    assert inner is not None
    inner.setScriptName("Inner")

    # the expressions read no parameter, so the renamed nodes have no listener
    own = group.createIntParam("own", "Own")
    own.setExpression("len(thisNode.getScriptName())", False, 0)
    child = inner.createIntParam("child", "Child")
    child.setExpression("len(thisGroup.getScriptName())", False, 0)
    assert own.getValueAtTime(1) == len("Outer"), own.getValueAtTime(1)
    assert child.getValueAtTime(1) == len("Outer"), child.getValueAtTime(1)

    group.setScriptName("RenamedOuter")
    assert app.getNode("RenamedOuter.Inner") is not None

    # a frame that was never evaluated, so that the expressions run again
    for param in (own, child):
        value = param.getValueAtTime(2)
        assert value == len("RenamedOuter"), "%s = %s after the rename" % (param.getScriptName(), value)
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
