#!/usr/bin/env python3
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

"""Headless test of the exit status of NatronRenderer when a render fails.

Usage: run_render_failure_status_test.py /path/to/NatronRenderer

NatronRenderer runs a script that builds Read -> Write1 on frames 1-3, then renders it.
- With an output directory that exists, the 3 frames are written and the exit status is 0.
- With an output path that cannot be created (its directory is a regular file), the render fails,
  no frame is written, and the exit status must not be 0: it was 0, so that a render farm manager
  took the failed render for a success.

Exit status: 0 on success, 77 if skipped (the Read/Write PNG plug-ins of openfx-io are not
available: set OFX_PLUGIN_PATH), 1 on failure.
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from run_gui_smoke_test import writePNG  # noqa: E402

SKIP = 77
TIMEOUT = 300

SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_FAILURE_TEST_DIR"]
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
else:
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(os.path.join(OUT, "input.png").replace("\\\\", "/"))
    write = app.createNode("fr.inria.built-in.Write")
    write.setScriptName("Write1")
    write.connectInput(0, read)
    write.getParam("filename").set(os.path.join(OUT, os.environ["NATRON_FAILURE_TEST_OUTPUT"], "out_###.png").replace("\\\\", "/"))
    write.getParam("frameRange").set(2)  # manual
    write.getParam("firstFrame").set(1)
    write.getParam("lastFrame").set(3)
"""


def run(renderer, tmp, outputName):
    """Returns (status, output, number of frames written)."""
    out = os.path.join(tmp, outputName + ".dir")
    os.makedirs(out)
    writePNG(os.path.join(out, "input.png"))
    if outputName == "frames":
        os.makedirs(os.path.join(out, outputName))
    else:
        # a regular file where the output directory should be
        with open(os.path.join(out, outputName), "w") as f:
            f.write("not a directory")
    script = os.path.join(out, "script.py")
    with open(script, "w") as f:
        f.write(SCRIPT)
    env = dict(os.environ, NATRON_FAILURE_TEST_DIR=out, NATRON_FAILURE_TEST_OUTPUT=outputName)
    try:
        proc = subprocess.run([renderer, "--no-settings", script], env=env, stdin=subprocess.DEVNULL,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return None, "timeout", 0
    framesDir = os.path.join(out, outputName)
    frames = len([f for f in os.listdir(framesDir) if f.startswith("out_")]) if os.path.isdir(framesDir) else 0
    return proc.returncode, proc.stdout.decode(errors="replace"), frames


def main():
    if len(sys.argv) != 2:
        sys.stderr.write(__doc__)
        return 1
    renderer = sys.argv[1]
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        status, output, frames = run(renderer, tmp, "frames")
        if status == SKIP:
            print("skipped: no PNG reader or writer (set OFX_PLUGIN_PATH)")
            return SKIP
        if status != 0 or frames != 3:
            print("FAILED: the render to an existing directory should write 3 frames and exit with status 0: "
                  "status %s, %d frames\n%s" % (status, frames, output[-3000:]))
            ok = False

        status, output, frames = run(renderer, tmp, "blocked")
        if status is None or status == 0:
            print("FAILED: the render to a path that cannot be created should not exit with status 0: "
                  "status %s, %d frames\n%s" % (status, frames, output[-3000:]))
            ok = False
        elif frames != 0:
            print("FAILED: the render to a path that cannot be created wrote %d frames" % frames)
            ok = False
    if ok:
        print("OK")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
