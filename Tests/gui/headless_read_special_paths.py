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

"""Headless test: EXR and PNG files in directories with special names are written and read (#848).

For each directory, a Write node writes an EXR and a PNG file there (from a PNG file in a plain
directory; if it fails, the file written to the plain directory is copied there), then a Read node
reads each one back: its region of definition must be the size of the image (a reader that cannot
read its file shows "0x0" in the viewer), and rendering it to a plain directory must write a file.
EXR files are read by the default reader and by each EXR reader of openfx-io (ReadEXR uses
OpenEXR, ReadOIIO OpenImageIO).

The directories: a '#' or "%02d" in the name (they were taken as frame numbers: "Frog #01/a.exr"
was read as "Frog 101/a.exr", #848), '@', spaces, non-ASCII characters, a path longer than the
260 characters of MAX_PATH (not on Windows, where NatronRenderer is not long-path aware, #2039),
and on Windows the same directory through a UNC path (//localhost/C$/...).

Variables still work: a sequence "Frog #01/seq.####.png" (the frame number in the file name), and a
"%V" in a directory name, which is the view name ("views Main/").

Skipped without the PNG and EXR readers and writers of openfx-io (set OFX_PLUGIN_PATH), or
without an OpenColorIO configuration for the writers (set OCIO).
"""

import os
import shutil
import struct
import sys
import tempfile
import zlib

from NatronEngine import natron

import natronheadless

WIDTH, HEIGHT = 8, 4
DIRECTORIES = (
    "Frog #01",
    "shot %02d",
    "take @@",
    "with spaces",
    "Frög été 日本",
)
EXR_READERS = ("fr.inria.openfx.ReadEXR", "fr.inria.openfx.ReadOIIO")
LONG_PATH = 300  # characters: more than MAX_PATH (260) on Windows


def write_png(path):
    """Writes an opaque 8-bit RGB PNG file of WIDTH x HEIGHT pixels."""
    row = b"\0" + bytes((200, 128, 64)) * WIDTH

    def chunk(tag, data):
        crc = zlib.crc32(tag + data) & 0xffffffff
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(row * HEIGHT)))
        f.write(chunk(b"IEND", b""))


def long_directory(out):
    """Returns a directory under out whose path is longer than LONG_PATH characters."""
    path = out
    while len(path) <= LONG_PATH:
        path += "/" + "long_directory_name_%02d" % (len(path) % 100)
    return path


def unc_path(path):
    """Returns the UNC path (//localhost/C$/...) of a Windows path on a local drive, or None."""
    if sys.platform != "win32" or len(path) < 2 or path[1] != ":":
        return None
    return "//localhost/%s$%s" % (path[0].upper(), path[2:])


def read_back(app, read, reader, path, plain):
    """Checks a Read node: its size, and that rendering it writes a file.

    Args:
        app: The Natron app.
        read: The Read node.
        reader: The name of the reader, for the messages.
        path: The file name of the Read node, for the messages.
        plain: A plain directory, where the image read is rendered.

    Returns:
        A description of what went wrong, or None.
    """
    rod = read.getRegionOfDefinition(1, 0)
    size = (rod.x2 - rod.x1, rod.y2 - rod.y1)
    if size != (WIDTH, HEIGHT):
        return "%s (%s) is %gx%g, expected %dx%d" % (path, reader, size[0], size[1], WIDTH, HEIGHT)
    copy = "%s/copy_%d.png" % (plain, len(os.listdir(plain)))
    write_copy = app.createWriter(copy)
    write_copy.connectInput(0, read)
    app.render(write_copy, 1, 1)
    if not os.path.isfile(copy):
        return "rendering %s (%s) wrote nothing" % (path, reader)
    return None


def write_and_read(app, source, directory, extension, plain):
    """Writes source to directory/image.<extension>, then reads it back.

    The file is read by a Read node (the default reader of the extension) and, for EXR, by each
    EXR reader of openfx-io on its own (ReadEXR uses OpenEXR, ReadOIIO OpenImageIO).

    Args:
        app: The Natron app.
        source: The Read node of the input image.
        directory: The directory to write to and read from, with '/' separators.
        extension: "exr" or "png".
        plain: A plain directory, where the images read back are rendered.

    Returns:
        The descriptions of what went wrong.
    """
    path = "%s/image.%s" % (directory, extension)
    write = app.createWriter(path)
    if write is None:
        return ["no Write node for " + path]
    write.connectInput(0, source)
    app.render(write, 1, 1)
    problems = []
    if not os.path.isfile(path):
        problems.append("the Write node did not write " + path)
        # still check the readers, on a copy of the file written to the plain directory
        shutil.copyfile("%s/reference.%s" % (plain, extension), path)
    readers = [("default reader", app.createReader(path))]
    if extension == "exr":
        for plugin_id in EXR_READERS:
            if plugin_id in natron.getPluginIDs():
                read = app.createNode(plugin_id)
                read.getParam("filename").set(path)
                readers.append((plugin_id, read))
            else:
                print("no %s plug-in: it is not tested" % plugin_id)
    for reader, read in readers:
        if read is None:
            problems.append("no %s for %s" % (reader, path))
            continue
        problem = read_back(app, read, reader, path, plain)
        if problem:
            problems.append(problem)
    return problems


def check_variables(app, source, out, plain):
    """Checks that the frame number of a file name, and %V in a directory name, are still replaced.

    Args:
        app: The Natron app.
        source: The Read node of the input image.
        out: The directory in which the directories of the files are created.
        plain: A plain directory, where the images read back are rendered.

    Returns:
        The descriptions of what went wrong.
    """
    problems = []
    sequence = out + "/Frog #01/seq.####.png"
    write = app.createWriter(sequence)
    natronheadless.check(write is not None, "no Write node for " + sequence)
    write.connectInput(0, source)
    app.render(write, 1, 2)
    for frame in (1, 2):
        written = "%s/Frog #01/seq.%04d.png" % (out, frame)
        if not os.path.isfile(written):
            problems.append("rendering %s did not write %s" % (sequence, written))
    read = app.createReader(sequence)
    natronheadless.check(read is not None, "no Read node for " + sequence)
    rod = read.getRegionOfDefinition(2, 0)
    if (rod.x2 - rod.x1, rod.y2 - rod.y1) != (WIDTH, HEIGHT):
        problems.append("frame 2 of %s is %gx%g" % (sequence, rod.x2 - rod.x1, rod.y2 - rod.y1))

    views = out + "/views %V/image.png"
    view_name = app.getViewNames()[0]
    # the Write node does not create the directory of a view
    os.makedirs("%s/views %s" % (out, view_name))
    write = app.createWriter(views)
    natronheadless.check(write is not None, "no Write node for " + views)
    write.connectInput(0, source)
    app.render(write, 1, 1)
    written = "%s/views %s/image.png" % (out, view_name)
    if not os.path.isfile(written):
        problems.append("rendering %s did not write %s" % (views, written))
    else:
        read = app.createReader(views)
        natronheadless.check(read is not None, "no Read node for " + views)
        problem = read_back(app, read, "default reader", views, plain)
        if problem:
            problems.append(problem)
    return problems


def main(app):
    """Writes and reads an EXR and a PNG file in each directory; fails with the list of problems."""
    natronheadless.require_plugins("fr.inria.openfx.ReadPNG", "fr.inria.openfx.WritePNG",
                                   "fr.inria.openfx.ReadOIIO", "fr.inria.openfx.WriteOIIO")
    with tempfile.TemporaryDirectory(prefix="natron_special_paths_",
                                     ignore_cleanup_errors=True) as tmp:
        out = os.path.realpath(tmp).replace("\\", "/")
        plain = out + "/plain"
        os.makedirs(plain)
        write_png(out + "/input.png")
        source = app.createReader(out + "/input.png")
        natronheadless.check(source is not None, "no Read node for the input")

        for extension in ("png", "exr"):
            reference = "%s/reference.%s" % (plain, extension)
            write = app.createWriter(reference)
            natronheadless.check(write is not None, "no Write node for " + reference)
            write.connectInput(0, source)
            app.render(write, 1, 1)
            natronheadless.check_rendered(reference)  # skips without OCIO

        directories = [out + "/" + d for d in DIRECTORIES]
        if sys.platform == "win32":
            # NatronRenderer.exe is not long-path aware (no longPathAware manifest): even Python's
            # os.makedirs() fails there with "The filename or extension is too long" (#2039)
            print("Windows: the path longer than MAX_PATH is not tested (#2039)")
        else:
            directories.append(long_directory(out))
        unc = unc_path(out + "/" + DIRECTORIES[0])
        for d in directories:
            os.makedirs(d)
        if unc is not None and os.path.isdir(unc):
            directories.append(unc)
        elif unc is not None:
            print("%s is not accessible: the UNC path is not tested" % unc)

        problems = check_variables(app, source, out, plain)
        for extension in ("png", "exr"):
            for d in directories:
                found = write_and_read(app, source, d, extension, plain)
                problems += found
                if not found:
                    print("ok: %s/image.%s" % (d, extension))
        natronheadless.check(not problems, "\n".join(problems))


natronheadless.run(main)
