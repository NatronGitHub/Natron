#!/usr/bin/env python3
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

"""Headless test of the -i and -w options of NatronRenderer on a project whose Read and Write have no file (#864).

Usage: run_renderer_cl_read_write_test.py /path/to/NatronRenderer [--debugger]

A NatronRenderer -t script saves a project with Read -> Write, both without a file name. Then
"NatronRenderer project.ntp -i Read plate.####.png -w Write out.####.png 12-14" must write the
frames 12 to 14 of the sequence plate.0010.png to plate.0020.png (#864: it read frame 0, "Cannot load frame", or wrote
the same frame for every frame). The frames of the sequence are all different (frame N is N
pixels wide), so the frames written must be too.

Exit status: 0 on success, 77 if skipped (without OCIO, or if the Read/Write PNG plug-ins of
openfx-io are not available: set OFX_PLUGIN_PATH), 1 on failure. --debugger runs NatronRenderer
under natron-lldb.sh, or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

import natronrunner

TIMEOUT = 120  # seconds per run; two runs stay below the ctest TIMEOUT (660)
SEQUENCE = range(10, 21)  # the frames of plate.####.png
FRAMES = [12, 13, 14]  # the frames rendered

SAVE_SCRIPT = """
import os
import sys
from NatronEngine import natron
OUT = os.environ["NATRON_CL_READ_WRITE_TEST_DIR"]
ids = natron.getPluginIDs()
hasReader = any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids)
if not hasReader or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
else:
    # Read has no file, so it has no decoder (ReadNodePrivate::createDefaultReadNode() destroys the default one):
    # -i creates one, ReadPNG here, ReadOIIO for the EXR files of the report. The new decoder shares the file
    # parameter of the Read node, which -i already set: it must still get the instance changed action for its
    # file, which sets its frame range (ReadNodePrivate::createReadNode(); without it, frame 0 was read).
    read = app.createNode("fr.inria.built-in.Read")
    read.setScriptName("Read")
    write = app.createNode("fr.inria.built-in.Write")
    write.setScriptName("Write")
    write.connectInput(0, read)
    app.saveProjectAs(os.path.join(OUT, "project.ntp"))
    sys.stdout.write("SAVED\\n"); sys.stdout.flush()
"""


def run_and_print(cmd, env):
    """Runs cmd and prints its output: returns (exit status, output), or None on a timeout."""
    status, out = natronrunner.run(cmd, env, TIMEOUT)
    sys.stdout.write(out)
    if status is None:
        print("FAILED: timeout after %d s" % TIMEOUT)
        return None
    print("exit status %d" % status)
    return status, out


def read_file(path):
    with open(path, "rb") as f:
        return f.read()


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    if not os.environ.get("OCIO"):
        # the Write node fails without an OCIO config ("Invalid OCIO config"), as for the Tests binary
        print("renderer -i/-w test skipped: set OCIO to an OpenColorIO config")
        return natronrunner.SKIP
    cmd_wrapper = natronrunner.wrapper(argv[2:])
    out_dir = tempfile.mkdtemp(prefix="natron-cl-read-write-test-")
    env = natronrunner.natron_env(out_dir, NATRON_CL_READ_WRITE_TEST_DIR=out_dir)
    project = os.path.join(out_dir, "project.ntp")
    try:
        os.mkdir(os.path.join(out_dir, "plate"))
        for frame in SEQUENCE:
            natronrunner.write_png(os.path.join(out_dir, "plate", "plate.%04d.png" % frame), frame, 8)
        script = os.path.join(out_dir, "save_project.py")
        with open(script, "w") as f:
            f.write(SAVE_SCRIPT)
        ret = run_and_print(cmd_wrapper + [argv[1], "--no-settings", "-t", script], env)
        if ret is None:
            return 1
        status, out = ret
        if "SKIP:" in out:
            print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
            return natronrunner.SKIP
        if status != 0 or "SAVED" not in out or not os.path.exists(project):
            print("FAILED: the project was not saved")
            return 1

        frame_range = "%d-%d" % (FRAMES[0], FRAMES[-1])
        plate = os.path.join(out_dir, "plate", "plate.####.png")
        ret = run_and_print(cmd_wrapper + [argv[1], "--no-settings", project, "-i", "Read", plate,
                                           "-w", "Write", os.path.join(out_dir, "out.####.png"), frame_range], env)
        if ret is None:
            return 1
        status, out = ret
        ok = True
        if status != 0:
            print("FAILED: exit status %d instead of 0" % status)
            ok = False
        if "Cannot load frame" in out or "Error caught while rendering" in out:
            print("FAILED: the render reported an error")
            ok = False
        written = sorted(f for f in os.listdir(out_dir) if f.startswith("out.") and f.endswith(".png"))
        expected = ["out.%04d.png" % frame for frame in FRAMES]
        if written != expected:
            print("FAILED: the render wrote [%s] instead of [%s]" % (" ".join(written), " ".join(expected)))
            ok = False
        elif len(set(read_file(os.path.join(out_dir, f)) for f in written)) != len(written):
            # (WritePNG dithers with a seed that depends on the frame: the same input frame could give different
            # bytes only if a row's random start column falls inside the image, at most 20 of 1920 columns)
            print("FAILED: the frames written are not all different: the same frame of the sequence was read "
                  "for each")
            ok = False
        if ok:
            print("renderer -i/-w test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
