# App.createNode(), createReader() and createWriter() with creation properties (#1827): run by NatronRenderer -t
# (see Tests/CMakeLists.txt). Windows builds dropped the properties argument ("too many arguments").
"""Creates nodes with the properties argument, positional and as the props= keyword."""

import os
import tempfile
import traceback

import NatronEngine
from NatronEngine import natron


def _name_property(name):
    return {"CreateNodeArgsPropNodeInitialName": NatronEngine.StringNodeCreationProperty(name)}


def _check_name(node, name):
    if node is None:
        raise AssertionError("no node created for %s" % name)
    if node.getScriptName() != name:
        raise AssertionError("script name %r, expected %r" % (node.getScriptName(), name))


status = 1
try:
    app = natron.getInstance(0)
    _check_name(app.createNode("fr.inria.built-in.Dot", -1, app, _name_property("PropsDot")), "PropsDot")
    _check_name(app.createNode("fr.inria.built-in.Dot", -1, app, props=_name_property("KeywordDot")), "KeywordDot")

    ids = natron.getPluginIDs()
    folder = tempfile.mkdtemp(prefix="natron-create-node-properties-")
    if "fr.inria.openfx.ReadOIIO" in ids:
        image = os.path.join(folder, "image.ppm")
        with open(image, "wb") as f:
            f.write(b"P6\n1 1\n255\n" + bytes(3))
        _check_name(app.createReader(image, app, _name_property("PropsRead")), "PropsRead")
        _check_name(app.createReader(image, app, props=_name_property("KeywordRead")), "KeywordRead")
    else:
        print("no ReadOIIO plug-in: createReader() is not tested")
    if "fr.inria.openfx.WriteOIIO" in ids:
        output = os.path.join(folder, "out.exr")
        _check_name(app.createWriter(output, app, _name_property("PropsWrite")), "PropsWrite")
        _check_name(app.createWriter(output, app, props=_name_property("KeywordWrite")), "KeywordWrite")
    else:
        print("no WriteOIIO plug-in: createWriter() is not tested")
    status = 0
except BaseException:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
