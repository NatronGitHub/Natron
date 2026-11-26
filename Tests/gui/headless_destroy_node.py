"""Effect.destroy() takes effect before it returns (#1805).

The node is no longer found, its script name is free, and it is no longer in its group. It used to wait until the
event loop ran, i.e. after the script. A node destroyed by one of its own callbacks is still destroyed later, since
the code that runs the callback still uses it.
"""

import natronheadless

DOT = "fr.inria.built-in.Dot"
GROUP = "fr.inria.built-in.Group"

# The callbacks that destroyed their node
callbacks_run = []


def check_destroyed(group, name):
    """Checks that no node named name is left in group (the app or a Group node)."""
    assert group.getNode(name) is None, name
    names = [n.getScriptName() for n in group.getChildren()]
    assert name not in names, (name, names)


def destroy_on_param_changed(thisParam, thisNode, thisGroup, app, userEdited):
    """onParamChanged callback: the node destroys itself when its destroyMe button is pressed."""
    del thisGroup, app, userEdited  # unused
    if thisParam.getScriptName() == "destroyMe":
        callbacks_run.append("onParamChanged")
        thisNode.destroy()


def destroy_on_input_changed(inputIndex, thisNode, thisGroup, app):
    """onInputChanged callback: the node destroys itself when an input is connected."""
    del thisGroup, app  # unused
    if thisNode.getInput(inputIndex) is not None:
        callbacks_run.append("onInputChanged")
        thisNode.destroy()


def connect(node, input_number, input_node):
    """Connects input_node to the input input_number of node, which must be free."""
    connected = node.connectInput(input_number, input_node)
    assert connected, (node.getScriptName(), input_number, input_node.getScriptName())


def main(app):
    dot = app.createNode(DOT)
    name = dot.getScriptName()
    dot.destroy()
    check_destroyed(app, name)
    dot.destroy()  # destroying a destroyed node does nothing
    # the name is free: the next Dot gets it
    again = app.createNode(DOT)
    assert again.getScriptName() == name, (again.getScriptName(), name)
    renamed = app.createNode(DOT)
    ok = renamed.setScriptName("Renamed")
    assert ok
    renamed.destroy()
    check_destroyed(app, "Renamed")
    ok = again.setScriptName("Renamed")
    assert ok

    # destroy() reconnects the outputs of the node to its input, destroy(False) doesn't
    source = app.createNode(DOT)
    middle = app.createNode(DOT)
    output = app.createNode(DOT)
    connect(middle, 0, source)
    connect(output, 0, middle)
    middle.destroy()
    assert output.getInput(0) == source, output.getInput(0)
    middle = app.createNode(DOT)
    connect(middle, 0, source)
    output.disconnectInput(0)
    connect(output, 0, middle)
    middle.destroy(False)
    assert output.getInput(0) is None, output.getInput(0).getScriptName()

    # a node in a Group, then the Group with the nodes in it
    group = app.createNode(GROUP)
    group_name = group.getScriptName()
    inner = app.createNode(DOT, -1, group)
    inner_name = inner.getScriptName()
    inner.destroy()
    check_destroyed(group, inner_name)
    app.createNode(DOT, -1, group)
    group.destroy()
    check_destroyed(app, group_name)

    # a node destroyed by its own callbacks (a Group, since a Dot has no callback parameters): this must not crash.
    # The destruction is deferred to the event loop, which NatronRenderer -t only runs after this script, so the
    # nodes are still there.
    self_destroying = app.createNode(GROUP)
    name = self_destroying.getScriptName()
    self_destroying.createButtonParam("destroyMe", "Destroy me")
    self_destroying.getParam("onParamChanged").set("destroy_on_param_changed")
    self_destroying.getParam("destroyMe").trigger()
    assert app.getNode(name) is not None, name
    self_destroying.destroy()  # the destruction is already pending: this does nothing
    self_destroying = app.createNode(GROUP)
    name = self_destroying.getScriptName()
    app.createNode("fr.inria.built-in.Input", -1, self_destroying)
    self_destroying.getParam("onInputChanged").set("destroy_on_input_changed")
    connect(self_destroying, 0, source)
    assert app.getNode(name) is not None, name
    assert callbacks_run == ["onParamChanged", "onInputChanged"], callbacks_run


natronheadless.run(main)
