# A PyPlug whose createInstance() sets a [Project] file path, for Tests/gui/headless_unsaved_project_path.py
# (found through NATRON_PLUGIN_PATH): Natron creates it also in an unsaved project (#1539).


def getPluginID():
    return "fr.natron.test.ProjectPath"


def getLabel():
    return "NatronTestProjectPath"


def getVersion():
    return 1


def getGrouping():
    return "Other"


def createInstance(app, group):
    param = group.createOutputFileParam("outFile", "Out")
    param.setValue("[Project]/pyplug.exr")
