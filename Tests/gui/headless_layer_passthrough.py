# -*- coding: utf-8 -*-
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

"""Headless test: the color plane goes through a node that writes to another layer.

Run by NatronRenderer -t. It exits with natron.quit(status): 0 on success, 1 on failure, 77 if
skipped (it needs Constant and Shuffle of openfx-misc and the PFM writer of openfx-io: set
OFX_PLUGIN_PATH).

    Constant (0.2, 0.4, 0.6) -> Shuffle that writes the green to a project layer -> Write

A multi-planar plug-in says which planes it produces; the other planes come from its pass-through
input. Natron added the color plane to the planes produced by every multi-planar plug-in, so the
Shuffle was asked to render the color plane, which it does not do when its output is another layer:
the color was black after it.

The second render reads the layer back into the red channel, to check that the layer is written
and that the color still goes through together with it.
"""

import os
import struct
import sys
import tempfile

import NatronEngine
from NatronEngine import natron

SKIP = 77
COLOUR = "uk.co.thefoundry.OfxImagePlaneColour"
NEEDED = ("net.sf.openfx.ConstantPlugin", "net.sf.openfx.ShufflePlugin", "fr.inria.openfx.WritePFM")


def set_option(node, name, option):
    param = node.getParam(name)
    assert param is not None, name
    options = list(param.getOptions())
    assert option in options, "%s: no option %s in %s" % (name, option, options)
    param.set(options.index(option))


def read_pfm_pixel(path):
    """Returns the pixel in the middle of a color PFM file (three floats)."""
    with open(path, "rb") as f:
        assert f.readline().strip() == b"PF", "not a color PFM file"
        width, height = [int(v) for v in f.readline().split()]
        scale = float(f.readline())
        f.seek((height // 2 * width + width // 2) * 12, os.SEEK_CUR)
        return struct.unpack(("<" if scale < 0 else ">") + "3f", f.read(12))


def render_pixel(app, write, node, path):
    write.disconnectInput(0)
    assert write.connectInput(0, node)
    write.getParam("filename").set(path)
    # the PFM writer (the default for .pfm is the OpenImageIO one, which writes PNM), three channels
    set_option(write, "encodingPluginChoice", "fr.inria.openfx.WritePFM")
    set_option(write, "outputComponents", "RGB")
    if os.path.exists(path):
        os.remove(path)
    app.render(write, 1, 1)
    assert os.path.isfile(path), "nothing was rendered to " + path
    return read_pfm_pixel(path)


def near(pixel, expected):
    return all(abs(a - b) < 1e-4 for a, b in zip(pixel, expected))


status = 1
try:
    ids = natron.getPluginIDs()
    if any(needed not in ids for needed in NEEDED):
        sys.stdout.write("SKIP: needs %s\n" % ", ".join(NEEDED))
        sys.stdout.flush()
        status = SKIP
    else:
        app = natron.getInstance(0)
        out = tempfile.mkdtemp(prefix="natron_layer_passthrough_").replace("\\", "/")
        app.addProjectLayer(NatronEngine.ImageLayer("testLayer", "testLayer", ["matte"]))

        constant = app.createNode("net.sf.openfx.ConstantPlugin")
        assert constant is not None
        color = constant.getParam("color")
        for dim, value in enumerate((0.2, 0.4, 0.6, 1.0)):
            color.setValue(value, dim)

        # writes the green of the Constant to the only channel of testLayer
        to_layer = app.createNode("net.sf.openfx.ShufflePlugin")
        assert to_layer.connectInput(0, constant)
        set_option(to_layer, "outputLayer", "testLayer")
        set_option(to_layer, "outputA", "B.%s.G" % COLOUR)

        write = app.createNode("fr.inria.built-in.Write")
        assert write is not None

        pixel = render_pixel(app, write, to_layer, out + "/color.pfm")
        assert near(pixel, (0.2, 0.4, 0.6)), "color after the Shuffle that writes a layer: %s" % (pixel,)

        # the layer in the red channel, green and blue from the color plane
        from_layer = app.createNode("net.sf.openfx.ShufflePlugin")
        assert from_layer.connectInput(0, to_layer)
        auto = from_layer.getParam("setGBAFromR")
        if auto is not None:
            auto.set(False)
        set_option(from_layer, "outputR", "B.testLayer.matte")
        pixel = render_pixel(app, write, from_layer, out + "/layer.pfm")
        assert near(pixel, (0.4, 0.4, 0.6)), "layer in red, color in green and blue: %s" % (pixel,)
        status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
