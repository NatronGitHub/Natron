# Effect.destroy() takes effect before it returns (#1805): the node is no longer found, its script name is
# free, and it is no longer in its group. It used to wait until the event loop ran, i.e. after the script.
# Run by NatronRenderer -t (see Tests/CMakeLists.txt).
from NatronEngine import natron

DOT = "fr.inria.built-in.Dot"
GROUP = "fr.inria.built-in.Group"


def check_destroyed(group, name):
    """Checks that no node named name is left in group (the app or a Group node)."""
    assert group.getNode(name) is None, name
    names = [n.getScriptName() for n in group.getChildren()]
    assert name not in names, (name, names)


status = 1
try:
    app = natron.getInstance(0)

    dot = app.createNode(DOT)
    name = dot.getScriptName()
    dot.destroy()
    check_destroyed(app, name)
    # the name is free: the next Dot gets it
    again = app.createNode(DOT)
    assert again.getScriptName() == name, (again.getScriptName(), name)
    renamed = app.createNode(DOT)
    assert renamed.setScriptName("Renamed")
    renamed.destroy()
    check_destroyed(app, "Renamed")
    assert again.setScriptName("Renamed")

    # destroy() reconnects the outputs of the node to its input, destroy(False) doesn't
    source = app.createNode(DOT)
    middle = app.createNode(DOT)
    output = app.createNode(DOT)
    assert middle.connectInput(0, source)
    assert output.connectInput(0, middle)
    middle.destroy()
    assert output.getInput(0) == source, output.getInput(0)
    middle = app.createNode(DOT)
    middle.connectInput(0, source)
    output.disconnectInput(0)
    assert output.connectInput(0, middle)
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
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
