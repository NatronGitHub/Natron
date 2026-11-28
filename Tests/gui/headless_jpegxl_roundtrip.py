# -*- coding: utf-8 -*-
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
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

"""Headless test: JPEG XL files are written and read back by the OpenImageIO nodes (#980).

Skipped without the PFM reader and writer and the OpenImageIO reader and writer of openfx-io (set
OFX_PLUGIN_PATH), without an OpenColorIO configuration for the writers (set OCIO), with an
openfx-io older than openfx-io#483 (whose WriteFFmpeg still offers .jxl files), or when the
OpenImageIO that openfx-io uses has no JPEG XL support (built without libjxl, or older than 2.6):
the OpenImageIO nodes get their file extensions from OpenImageIO. With NATRON_TEST_REQUIRE_JPEGXL=1
(the package workflows, whose OpenImageIO has libjxl and whose openfx-io is its master branch), the
test fails instead of skipping.

    Read gradient.pfm -> Write gradient.jxl (the default writer: WriteOIIO, 32-bit float, lossless)
    Read gradient.jxl -> Write back.pfm

The first Write must choose WriteOIIO by default and write a lossless floating-point JPEG XL file,
the second Read must read it with ReadOIIO, and the pixels of back.pfm must be those of
gradient.pfm. Every Read and Write uses the colorspace of the first Read (linear): the defaults
that the OpenImageIO nodes guess from the bit depth and the file are not what this test checks.
"""

import os
import struct
import tempfile

import natronheadless

NEEDED = ("fr.inria.openfx.ReadPFM", "fr.inria.openfx.WritePFM",
          "fr.inria.openfx.ReadOIIO", "fr.inria.openfx.WriteOIIO")
WIDTH, HEIGHT = 8, 4
# the points checked, (x, y) with y = 0 at the bottom: the corners and a pixel inside
POINTS = ((0, 0), (WIDTH - 1, 0), (0, HEIGHT - 1), (WIDTH - 1, HEIGHT - 1), (3, 2))
# a lossless 32-bit float file gives the floats of the PFM file back (colour() is in double precision); a
# 16-bit or half-float file, or a lossy one, would be off by more
TOLERANCE = 1e-6
# the signatures of a JPEG XL file: a bare codestream, or the ISO BMFF container
JXL_SIGNATURES = (b"\xff\x0a", b"\x00\x00\x00\x0cJXL \x0d\x0a\x87\x0a")
# set in the package workflows, where JPEG XL must work: no skip then
REQUIRED = os.environ.get("NATRON_TEST_REQUIRE_JPEGXL") == "1"


def colour(x, y):
    """The RGB colour of the pixel (x, y) of the gradient (y = 0 at the bottom)."""
    return ((x + 1) / (WIDTH + 1.), (y + 1) / (HEIGHT + 1.), 0.5)


def write_pfm(path):
    """Writes the gradient as an RGB PFM file (little-endian floats, bottom row first)."""
    with open(path, "wb") as f:
        f.write(b"PF\n%d %d\n-1.0\n" % (WIDTH, HEIGHT))
        for y in range(HEIGHT):
            for x in range(WIDTH):
                f.write(struct.pack("<3f", *colour(x, y)))


def render(app, read_path, write_path, encoder, colorspace=None, bit_depth=None):
    """Renders the file read_path to write_path with the writer plug-in encoder.

    Args:
        app: the App.
        read_path: the file read.
        write_path: the file written.
        encoder: the plug-in ID of the writer (the encodingPluginChoice option of the Write); None
            keeps the default writer, which must be WriteOIIO.
        colorspace: the OpenColorIO colorspace of the file read_path; None keeps the default of the
            Read. The Write uses the same colorspace.
        bit_depth: the bitDepth option of the Write; None keeps its default.

    Returns:
        The Read node.
    """
    read = app.createReader(read_path)
    natronheadless.check(read is not None, "no Read node for " + read_path)
    write = app.createNode("fr.inria.built-in.Write")
    natronheadless.check(write is not None, "no Write node")
    connected = write.connectInput(0, read)
    natronheadless.check(connected, "the Write node cannot be connected")
    write.getParam("filename").set(write_path)
    if encoder is None:
        default = write.getParam("encodingPluginID").get()
        natronheadless.check(default == "fr.inria.openfx.WriteOIIO",
                             "%s is written by %s by default, expected fr.inria.openfx.WriteOIIO"
                             % (write_path, default))
    else:
        natronheadless.set_option(write, "encodingPluginChoice", encoder)
    natronheadless.set_option(write, "outputComponents", "RGB")
    if bit_depth is not None:
        natronheadless.set_option(write, "bitDepth", bit_depth)
    if colorspace is not None:
        read.getParam("ocioInputSpace").set(colorspace)
    write.getParam("ocioOutputSpace").set(read.getParam("ocioInputSpace").get())
    app.render(write, 1, 1)
    natronheadless.check_rendered(write_path)
    return read


def jxl_writers(app, path):
    """The plug-in IDs that a Write node offers for path (the encodingPluginChoice options)."""
    write = app.createNode("fr.inria.built-in.Write")
    natronheadless.check(write is not None, "no Write node")
    write.getParam("filename").set(path)
    writers = list(write.getParam("encodingPluginChoice").getOptions())
    write.destroy()
    return writers


def roundtrip(app):
    """The test: raises Skipped if it cannot run here."""
    natronheadless.require_plugins(*NEEDED)
    # the Read nodes may keep their files open (Windows cannot remove them then)
    with tempfile.TemporaryDirectory(prefix="natron_jpegxl_roundtrip_", ignore_cleanup_errors=True) as tmp:
        out = tmp.replace("\\", "/")
        source, jxl, back = out + "/gradient.pfm", out + "/gradient.jxl", out + "/back.pfm"
        writers = jxl_writers(app, jxl)
        if "fr.inria.openfx.WriteFFmpeg" in writers:
            raise natronheadless.Skipped("WriteFFmpeg offers .jxl files: openfx-io is older than openfx-io#483")
        if "fr.inria.openfx.WriteOIIO" not in writers:
            raise natronheadless.Skipped("WriteOIIO does not write .jxl files: OpenImageIO has no JPEG XL support")
        write_pfm(source)

        read = render(app, source, jxl, None, bit_depth="32f")
        linear = read.getParam("ocioInputSpace").get()
        with open(jxl, "rb") as f:
            head = f.read(12)
        natronheadless.check(any(head.startswith(s) for s in JXL_SIGNATURES),
                             "%s is not a JPEG XL file (it starts with %r)" % (jxl, head))

        read = render(app, jxl, back, "fr.inria.openfx.WritePFM", colorspace=linear)
        decoder = read.getParam("decodingPluginID").get()
        natronheadless.check(decoder == "fr.inria.openfx.ReadOIIO",
                             "%s is read by %s, expected fr.inria.openfx.ReadOIIO" % (jxl, decoder))
        pixels = natronheadless.read_pfm_pixels(back, POINTS)
        for point, pixel in zip(POINTS, pixels):
            expected = colour(*point)
            natronheadless.check(all(abs(a - b) < TOLERANCE for a, b in zip(pixel, expected)),
                                 "pixel %r after the JPEG XL round trip: %r, expected %r" % (point, pixel, expected))


def main(app):
    """Runs the test; with NATRON_TEST_REQUIRE_JPEGXL=1, a skip is a failure."""
    try:
        roundtrip(app)
    except natronheadless.Skipped as e:
        natronheadless.check(not REQUIRED, "NATRON_TEST_REQUIRE_JPEGXL=1, but the test was skipped: %s" % e)
        raise


natronheadless.run(main)
