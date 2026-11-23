# A version 3 PyPlug, for Tests/gui/test_pyplug_export_version.py (found through NATRON_PLUGIN_PATH).


def getPluginID():
    return "fr.natron.test.ExportVersion"


def getLabel():
    return "NatronTestExportVersion"


def getVersion():
    return 3


def getGrouping():
    return "Other"


def createInstance(app, group):
    # Export as PyPlug needs an Output node in the group
    app.createNode("fr.inria.built-in.Output", 1, group)
