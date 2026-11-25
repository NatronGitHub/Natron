"""A Roto or RotoPaint node needs openfx-misc's Roto and Merge plug-ins (#1828).

They render the shapes of the node. Without them, app.createNode() of these nodes returns None. Before, the
Roto node was created, and its first shape (Roto.createBezier()) threw a C++ exception through Python:
"SystemError: unknown opcode", then NatronRenderer hung or crashed.

Skipped when openfx-misc is installed.
"""
from NatronEngine import natron

import natronheadless


def main(app):
    ids = natron.getPluginIDs()
    if "net.sf.openfx.RotoPlugin" in ids and "net.sf.openfx.MergePlugin" in ids:
        raise natronheadless.Skipped("openfx-misc is installed")
    for pluginID in ("fr.inria.built-in.Roto", "fr.inria.built-in.RotoPaint"):
        node = app.createNode(pluginID)
        assert node is None, f"createNode({pluginID}) returned {node!r}"
    # the app still works
    assert app.createNode("fr.inria.built-in.Dot") is not None, "createNode(Dot) failed"


natronheadless.run(main)
