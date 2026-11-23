# A PyPlug for Tests/gui/headless_pyplug_rebound_app.py (found through NATRON_PLUGIN_PATH).


def getPluginID():
    return "fr.natron.test.ReboundApp"


def getLabel():
    return "NatronTestReboundApp"


def getVersion():
    return 1


def getGrouping():
    return "Other"


def createInstance(app, group):
    # declared as an attribute of the group node, itself an attribute of the app object
    inner = app.createNode("fr.inria.built-in.Dot", 1, group)
    inner.setScriptName("Inner")
