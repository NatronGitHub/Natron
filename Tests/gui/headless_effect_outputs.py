# Effect.getOutputs() lists the nodes downstream of a node, and Effect.getContainerGroup() returns the group
# that contains it (#1797). Run by NatronRenderer -t (see Tests/CMakeLists.txt).
import traceback

import NatronEngine
from NatronEngine import natron


def check_outputs(app):
    """A node connected twice to the same output is listed once, in the order of the connections."""
    source = app.createNode("fr.inria.built-in.Dot")
    group = app.createNode("fr.inria.built-in.Group")
    dot = app.createNode("fr.inria.built-in.Dot")
    assert source is not None and group is not None and dot is not None, "createNode() failed"
    assert source.getOutputs() == []
    while group.getMaxInputCount() < 2:
        assert app.createNode("fr.inria.built-in.Input", -1, group) is not None, "createNode(Input) failed"
    assert group.connectInput(0, source), "group.connectInput(0) failed"
    assert group.connectInput(1, source), "group.connectInput(1) failed"
    assert dot.connectInput(0, source), "dot.connectInput(0) failed"
    outputs = source.getOutputs()
    assert all(isinstance(o, NatronEngine.Effect) for o in outputs), outputs
    assert outputs == [group, dot], [o.getScriptName() for o in outputs]
    assert dot.getOutputs() == []
    group.disconnectInput(0)
    assert source.getOutputs() == [group, dot], [o.getScriptName() for o in source.getOutputs()]
    group.disconnectInput(1)
    assert source.getOutputs() == [dot], [o.getScriptName() for o in source.getOutputs()]
    return group


def check_container_group(app, group):
    """A node in a group is contained in the group node, a top-level node in the App."""
    inner = app.createNode("fr.inria.built-in.Dot", -1, group)
    assert inner is not None, "createNode(Dot, group) failed"
    container = inner.getContainerGroup()
    assert isinstance(container, NatronEngine.Effect), container
    assert container == group, container.getScriptName()
    top = group.getContainerGroup()
    assert isinstance(top, NatronEngine.App), top
    assert group in top.getChildren()
    # the container is a valid group for createNode()
    sibling = app.createNode("fr.inria.built-in.Dot", -1, container)
    assert sibling is not None and sibling.getContainerGroup() == group


def main():
    status = 1
    try:
        app = natron.getInstance(0)
        group = check_outputs(app)
        check_container_group(app, group)
        status = 0
    except BaseException:
        traceback.print_exc()
    natron.quit(status)


main()
