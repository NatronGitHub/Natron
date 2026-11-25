# The same PyPlug as Tests/data/ofx-bundle-pyplugs/NatronTestBundlePyPlug.py (same ID and version), found through
# NATRON_PLUGIN_PATH by the TestsOfxBundlePyPlugOverride test (Tests/CMakeLists.txt): it overrides the copy in the
# OFX plug-in bundle (Tests/OfxBundlePyPlugs_Test.cpp).


def getPluginID():
    return "net.natron.test.BundlePyPlug"


def getLabel():
    return "BundlePyPlugOverride"


def getVersion():
    return 1


def getGrouping():
    return "Other"


def createInstance(app, group):
    inp = app.createNode("fr.inria.built-in.Input", -1, group)
    out = app.createNode("fr.inria.built-in.Output", -1, group)
    out.connectInput(0, inp)
