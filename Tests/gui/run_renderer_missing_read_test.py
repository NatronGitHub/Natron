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

"""Headless test of a render whose Read has a missing file (#1602) or no filename (#1131).

Usage: run_renderer_missing_read_test.py /path/to/NatronRenderer [--debugger]

NatronRenderer -b runs a script that creates Read (on a missing file, or with no filename)
-> Write1 (PNG, frames 1-3), and renders Write1:
- "error": the Read is on its default "On missing frame: Error". The render must fail
  ("Error caught while rendering"), write no frame and exit with status 1: it wrote three black
  frames and reported a successful render;
- "empty": the Read has an empty filename. The render must fail as for "error", and report
  "Filename empty": NatronRenderer crashed (#1131), then wrote black frames;
- "black": "On missing frame: Black". The render must write the three (black) frames and exit
  with status 0;
- "merge": the Read (Error) is the A input of a Merge whose B input is a Read that works. The
  render must fail and write no frame: Merge, which gets an empty region of definition for A, was
  the identity of B and wrote B (skipped without openfx-misc's Merge);
- "hold": the Read (Error) is on a sequence whose frame 2 is missing, and a FrameHold holds its
  frame 1. The render must write the three frames: the missing frame is never used (skipped
  without openfx-misc's FrameHold).

Exit status: 0 on success, 77 if skipped (without OCIO, or if the Read/Write PNG plug-ins of
openfx-io are not available: set OFX_PLUGIN_PATH), 1 on failure. --debugger runs NatronRenderer
under natron-lldb.sh, or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

from natronrunner import SKIP, natron_env, run, wrapper, write_png

MODE_SKIPPED = "mode skipped"
TIMEOUT = 120  # seconds per run; five runs stay below the ctest TIMEOUT (660)
FRAMES = [1, 2, 3]

SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_MISSING_READ_TEST_DIR"]
MODE = os.environ["NATRON_MISSING_READ_TEST_MODE"]
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
elif (MODE == "merge" and "net.sf.openfx.MergePlugin" not in ids) or (MODE == "hold" and "net.sf.openfx.FrameHold" not in ids):
    sys.stdout.write("SKIP MODE: no openfx-misc plug-in\\n"); sys.stdout.flush()
    natron.quit(0)
else:
    if MODE == "empty":
        read = app.createNode("fr.inria.built-in.Read")
    else:
        read = app.createReader(os.path.join(OUT, "seq" if MODE == "hold" else "missing", "plate.####.png"))
        onMissingFrame = read.getParam("onMissingFrame")
        onMissingFrame.setValue(onMissingFrame.getOptions().index("black" if MODE == "black" else "error"))
    output = read
    if MODE == "merge":
        output = app.createNode("net.sf.openfx.MergePlugin")
        output.connectInput(0, app.createReader(os.path.join(OUT, "input.png")))  # B
        output.connectInput(1, read)  # A
    elif MODE == "hold":
        output = app.createNode("net.sf.openfx.FrameHold")
        output.connectInput(0, read)
        output.getParam("firstFrame").setValue(1)
    write = app.createWriter(os.path.join(OUT, MODE + "_####.png"))
    write.setScriptName("Write1")
    write.connectInput(0, output)
    write.getParam("frameRange").setValue(2)  # manual
    write.getParam("firstFrame").setValue(@FIRST@)
    write.getParam("lastFrame").setValue(@LAST@)
""".replace("@FIRST@", str(FRAMES[0])).replace("@LAST@", str(FRAMES[-1]))


def run_mode(renderer, cmd_wrapper, out_dir, script, mode):
    """Runs the script with the given mode.

    Returns:
        True on success, False on failure, SKIP if the whole test is skipped (no PNG plug-ins), or
        MODE_SKIPPED if only this mode is (no openfx-misc plug-in).
    """
    env = natron_env(out_dir, NATRON_MISSING_READ_TEST_DIR=out_dir, NATRON_MISSING_READ_TEST_MODE=mode)
    print("Mode %s:" % mode)
    status, out = run(cmd_wrapper + [renderer, "--no-settings", "-b", script], env, TIMEOUT)
    sys.stdout.write(out)
    if status is None:
        print("FAILED (%s): timeout after %d s" % (mode, TIMEOUT))
        return False
    if "SKIP:" in out:
        return SKIP
    if "SKIP MODE: no openfx-misc plug-in" in [line.strip() for line in out.splitlines()]:
        print("skipped (%s): no openfx-misc plug-in" % mode)
        return MODE_SKIPPED
    print("NatronRenderer -b (%s) exited with status %d" % (mode, status))
    written = sorted(f for f in os.listdir(out_dir) if f.startswith(mode + "_") and f.endswith(".png"))
    failed = "Error caught while rendering" in out
    ok = True
    if mode in ("error", "empty", "merge"):
        if written:
            print("FAILED (%s): the render wrote %s, with an input that failed" % (mode, " ".join(written)))
            ok = False
        if not failed:
            print("FAILED (%s): the render did not report a failure" % mode)
            ok = False
        if status != 1:
            print("FAILED (%s): exit status %d instead of 1" % (mode, status))
            ok = False
        if mode == "empty" and "Filename empty" not in out:
            print("FAILED (empty): the render did not report the empty filename")
            ok = False
    else:
        expected = ["%s_%04d.png" % (mode, frame) for frame in FRAMES]
        if written != expected:
            print("FAILED (%s): the render wrote [%s] instead of [%s]" % (mode, " ".join(written), " ".join(expected)))
            ok = False
        if failed:
            print("FAILED (%s): the render reported a failure" % mode)
            ok = False
        if status != 0:
            print("FAILED (%s): exit status %d instead of 0" % (mode, status))
            ok = False
    return ok


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    if not os.environ.get("OCIO"):
        # the Write node fails without an OCIO config ("Invalid OCIO config"), as for the Tests binary
        print("renderer missing Read test skipped: set OCIO to an OpenColorIO config")
        return SKIP
    cmd_wrapper = wrapper(argv[2:])
    out_dir = tempfile.mkdtemp(prefix="natron-missing-read-test-")
    try:
        write_png(os.path.join(out_dir, "input.png"), 64, 48)
        os.mkdir(os.path.join(out_dir, "seq"))
        for frame in (1, 3):  # frame 2 is missing
            write_png(os.path.join(out_dir, "seq", "plate.%04d.png" % frame), 64, 48)
        script = os.path.join(out_dir, "missing_read_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        ok = True
        skipped = []
        for mode in ("error", "empty", "black", "merge", "hold"):
            ret = run_mode(argv[1], cmd_wrapper, out_dir, script, mode)
            if ret == MODE_SKIPPED:
                skipped.append(mode)
                continue
            if ret == SKIP:
                print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
                return SKIP
            if not ret:
                ok = False
        if ok:
            print("renderer missing Read test passed" + (" (skipped: %s)" % ", ".join(skipped) if skipped else ""))
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
