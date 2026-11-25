# A PyPlug that ships inside an OFX plug-in bundle, in NatronTestPlugin.ofx.bundle/Contents/Resources/PyPlugs/
# (copied there by Tests/CMakeLists.txt and Tests/OfxTestPlugin/OfxTestPlugin.pro), for
# Tests/OfxBundlePyPlugs_Test.cpp. Its icon is next to it.


def getPluginID():
    return "net.natron.test.BundlePyPlug"


def getLabel():
    return "BundlePyPlug"


def getVersion():
    return 1


def getIconPath():
    return "NatronTestBundlePyPlug.png"


def getGrouping():
    return "Other"


def createInstance(app, group):
    inp = app.createNode("fr.inria.built-in.Input", -1, group)
    out = app.createNode("fr.inria.built-in.Output", -1, group)
    out.connectInput(0, inp)
