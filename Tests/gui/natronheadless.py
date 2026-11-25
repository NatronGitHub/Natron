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

"""Helpers shared by the headless tests (Tests/gui/headless_*.py).

Each test is run by "NatronRenderer -t headless_<name>.py" (ctest test headless.<name>, see
Tests/CMakeLists.txt), which puts the directory of the script on sys.path. A test defines main(app)
and ends with run(main), which exits with natron.quit(status): 0 on success, SKIP (77) if main
raised Skipped (e.g. a plug-in it needs is not on OFX_PLUGIN_PATH), 1 on failure.

Engine only (NatronEngine): NatronRenderer has no GUI. The GUI tests use natrontest.py.
"""

import os
import struct
import sys
import traceback

from NatronEngine import natron

SKIP = 77  # the exit status of a skipped test (SKIP_RETURN_CODE in Tests/CMakeLists.txt)


class Skipped(Exception):
    """The test cannot run here; the message says why."""


def check(condition, message=""):
    """Like assert, but not removed by python -O."""
    if not condition:
        raise AssertionError(message)


def require_plugins(*plugin_ids):
    """Raises Skipped if one of plugin_ids is not available."""
    ids = natron.getPluginIDs()
    missing = [p for p in plugin_ids if p not in ids]
    if missing:
        raise Skipped("plug-ins %s not found (OFX_PLUGIN_PATH=%s)"
                      % (", ".join(missing), os.environ.get("OFX_PLUGIN_PATH", "")))


def check_rendered(path):
    """After app.render(): fails if nothing was written to path, or skips without OpenColorIO.

    The writers of openfx-io fail without an OpenColorIO configuration (set OCIO).
    """
    if not os.path.isfile(path) and not os.path.isfile(os.environ.get("OCIO", "")):
        raise Skipped("nothing was rendered and there is no OpenColorIO configuration: set OCIO")
    check(os.path.isfile(path), "nothing was rendered to " + path)


def read_pfm_pixels(path, points=None):
    """Returns the pixels at points ((x, y), y = 0 at the bottom) of a PFM image, as tuples of floats.

    Without points: the pixel in the middle of the image.
    """
    with open(path, "rb") as f:
        magic = f.readline().strip()
        check(magic in (b"PF", b"Pf"), "%s is not a PFM file" % path)
        channels = 3 if magic == b"PF" else 1
        width, height = (int(v) for v in f.readline().split())
        endian = "<" if float(f.readline()) < 0 else ">"
        start = f.tell()
        if points is None:
            points = [(width // 2, height // 2)]
        pixels = []
        for x, y in points:
            check(0 <= x < width and 0 <= y < height,
                  "%r is outside the %dx%d image %s" % ((x, y), width, height, path))
            f.seek(start + (y * width + x) * channels * 4)
            pixels.append(struct.unpack("%s%df" % (endian, channels), f.read(channels * 4)))
        return pixels


def run(main):
    """Runs main(app) and exits NatronRenderer with its status (see the module docstring)."""
    status = 1
    try:
        main(natron.getInstance(0))
        status = 0
    except Skipped as e:
        print("SKIP: %s" % e)
        status = SKIP
    except Exception:  # pylint: disable=broad-except
        traceback.print_exc()
    sys.stdout.flush()
    natron.quit(status)
