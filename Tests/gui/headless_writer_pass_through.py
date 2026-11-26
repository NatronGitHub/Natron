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

"""Headless test: a Write node writes its files only when it is rendered itself (#815).

Skipped without the PNG reader and writer of openfx-io (set OFX_PLUGIN_PATH), or without an
OpenColorIO configuration for the writer (set OCIO).

    Read plate.####.png -> WriteB b.####.png -> WriteS s.png -> Dot -> WriteA a.####.png

Rendering WriteA pulls the images through WriteB and WriteS, which pass their input through. They
also wrote their own files: rendering one Write node overwrote the files of the others (#815), and
each frame of that render rewrote the single file of WriteS. Then WriteB and WriteS are rendered,
to check that they still write their files. The red of frame N of the sequence is 10 N, so the
colour of a file tells which frame of the sequence it holds.
"""

import os
import struct
import tempfile
import zlib

import natronheadless

FRAMES = (11, 12, 13)  # the frames of the sequence, all rendered
NEEDED = ("fr.inria.openfx.ReadPNG", "fr.inria.openfx.WritePNG")


def write_png(path, frame, width=8, height=4):
    """Writes an opaque 8-bit RGB PNG file of a uniform colour, whose red is 10 * frame."""
    row = b"\0" + bytes((10 * frame, 128, 64)) * width

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(row * height)))
        f.write(chunk(b"IEND", b""))


def png_frame(path):
    """Returns the frame of the sequence that a PNG file written by Natron holds: its first red, divided by 10."""
    with open(path, "rb") as f:
        data = f.read()
    natronheadless.check(data[:8] == b"\x89PNG\r\n\x1a\n", path + " is not a PNG file")
    pos, idat = 8, b""
    while pos < len(data):
        length, tag = struct.unpack(">I4s", data[pos:pos + 8])
        if tag == b"IHDR":
            depth = data[pos + 16]
            natronheadless.check(depth == 8, "%s: %d bits per channel, expected 8" % (path, depth))
        elif tag == b"IDAT":
            idat += data[pos + 8:pos + 8 + length]
        pos += 12 + length
    # the first byte of a row is its filter type; whatever the filter, the first pixel of the first row is stored as is
    red = zlib.decompress(idat)[1]
    return int(round(red / 10.0))


def written(out):
    """Returns the sorted names of the files written by the Write nodes."""
    return sorted(f for f in os.listdir(out) if f.endswith(".png") and not f.startswith("plate."))


def remove_written(out):
    """Removes the files written by the Write nodes from the directory out."""
    for f in written(out):
        os.remove(os.path.join(out, f))


def render(app, write, first, last, out):
    """Renders the frames first to last of write, after removing the files written before.

    Returns:
        The names of the files written, sorted.

    Raises:
        natronheadless.Skipped: nothing was written, and OCIO is not an OpenColorIO configuration.
    """
    remove_written(out)
    app.render(write, first, last)
    files = written(out)
    if not files and not os.path.isfile(os.environ.get("OCIO", "")):
        # the writers of openfx-io fail without an OpenColorIO configuration
        raise natronheadless.Skipped("nothing was rendered and there is no OpenColorIO configuration: set OCIO")
    return files


def connect(node, input_node):
    """Connects input_node to the input 0 of node; fails if it cannot."""
    connected = node.connectInput(0, input_node)
    natronheadless.check(connected, "%s cannot be connected to %s" % (node.getScriptName(), input_node.getScriptName()))


def create_writer(app, path, name):
    """Returns a new Write node named name that writes path."""
    write = app.createWriter(path)
    natronheadless.check(write is not None, "no Write node for " + path)
    write.setScriptName(name)
    return write


def main(app):
    """Renders WriteA, then WriteB and WriteS, and checks the files each one writes."""
    natronheadless.require_plugins(*NEEDED)
    with tempfile.TemporaryDirectory(prefix="natron_writer_pass_through_", ignore_cleanup_errors=True) as tmp:
        out = tmp.replace("\\", "/")
        for frame in FRAMES:
            write_png("%s/plate.%04d.png" % (out, frame), frame)

        read = app.createReader(out + "/plate.####.png")
        natronheadless.check(read is not None, "no Read node")
        write_b = create_writer(app, out + "/b.####.png", "WriteB")
        connect(write_b, read)
        write_s = create_writer(app, out + "/s.png", "WriteS")
        connect(write_s, write_b)
        dot = app.createNode("fr.inria.built-in.Dot")
        natronheadless.check(dot is not None, "no Dot node")
        connect(dot, write_s)
        write_a = create_writer(app, out + "/a.####.png", "WriteA")
        connect(write_a, dot)

        first, last = FRAMES[0], FRAMES[-1]
        files = render(app, write_a, first, last, out)
        expected = ["a.%04d.png" % f for f in FRAMES]
        natronheadless.check(files == expected,
                             "rendering WriteA wrote %r, expected %r: WriteB and WriteS must not write their files "
                             "when WriteA pulls images through them" % (files, expected))
        frames = [png_frame("%s/a.%04d.png" % (out, f)) for f in FRAMES]
        natronheadless.check(frames == list(FRAMES),
                             "WriteA wrote frames %r of the sequence, expected %r" % (frames, list(FRAMES)))

        files = render(app, write_b, first, last, out)
        expected = ["b.%04d.png" % f for f in FRAMES]
        natronheadless.check(files == expected, "rendering WriteB wrote %r, expected %r" % (files, expected))

        files = render(app, write_s, 12, 12, out)
        natronheadless.check(files == ["s.png"], "rendering WriteS at frame 12 wrote %r, expected ['s.png']" % (files,))
        natronheadless.check(png_frame(out + "/s.png") == 12,
                             "WriteS wrote frame %d of the sequence, expected 12" % png_frame(out + "/s.png"))


natronheadless.run(main)
