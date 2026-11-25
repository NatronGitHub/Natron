# Without the Roto and Merge plug-ins of openfx-misc, which render the shapes of the Roto and RotoPaint
# nodes, app.createNode() of these nodes returns None (#1828). The Roto node was created, and its first
# shape (Roto.createBezier()) threw a C++ exception through Python: "SystemError: unknown opcode", then
# NatronRenderer hung or crashed. Run by NatronRenderer -t (see Tests/CMakeLists.txt); exits with 77
# (skipped) when openfx-misc is installed.
from NatronEngine import natron

SKIP = 77

status = 1
try:
    ids = natron.getPluginIDs()
    if "net.sf.openfx.RotoPlugin" in ids and "net.sf.openfx.MergePlugin" in ids:
        print("openfx-misc is installed: skipped")
        status = SKIP
    else:
        app = natron.getInstance(0)
        for pluginID in ("fr.inria.built-in.Roto", "fr.inria.built-in.RotoPaint"):
            node = app.createNode(pluginID)
            assert node is None, "createNode(%s) returned %r" % (pluginID, node)
        # the app still works
        assert app.createNode("fr.inria.built-in.Dot") is not None, "createNode(Dot) failed"
        status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
