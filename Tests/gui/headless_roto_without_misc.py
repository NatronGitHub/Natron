"""A Roto or RotoPaint node needs openfx-misc's Roto and Merge plug-ins (#1828).

They render the shapes of the node. Without them, app.createNode() of these nodes returns None. Before, the
Roto node was created, and its first shape (Roto.createBezier()) threw a C++ exception through Python:
"SystemError: unknown opcode", then NatronRenderer hung or crashed.

Run by NatronRenderer -t (see Tests/CMakeLists.txt). Exits with 0 on success, 1 on failure, 77 (skipped)
when openfx-misc is installed.
"""
import traceback

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
            assert node is None, f"createNode({pluginID}) returned {node!r}"
        # the app still works
        assert app.createNode("fr.inria.built-in.Dot") is not None, "createNode(Dot) failed"
        status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
