# A PyPlug can be created after a script rebinds the global "app" (#1525): run by NatronRenderer -t (see
# Tests/CMakeLists.txt), with Tests/data/pyplugs in NATRON_PLUGIN_PATH. In background mode, the nodes are
# declared on "app"; the PyPlug creation looked its group node up on "app1", another App object once "app" is
# rebound, and failed with "AttributeError: ... has no attribute 'NatronTestReboundApp1'".
from NatronEngine import natron

PYPLUG = "fr.natron.test.ReboundApp"

status = 1
try:
    assert PYPLUG in natron.getPluginIDs(), "%s not found: is Tests/data/pyplugs in NATRON_PLUGIN_PATH?" % PYPLUG
    app = natron.getInstance(0)  # a new App object for the same project: not app1
    assert app is not app1
    node = app.createNode(PYPLUG)
    assert node is not None, "createNode(%s) failed" % PYPLUG
    name = node.getScriptName()
    # the variable that #1525 is about: the group node is declared on the rebound app
    assert getattr(app, name, None) is not None, "%s is not declared on app" % name
    assert app.getNode(name + ".Inner") is not None, "the PyPlug did not create its Inner node"
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
