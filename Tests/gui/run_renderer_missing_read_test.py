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

"""Headless test of a render whose Read node has a missing file (#1602).

Usage: run_renderer_missing_read_test.py /path/to/NatronRenderer [--debugger]

NatronRenderer -b runs a script that creates Read (on a file that does not exist) -> Write1
(PNG, frames 1-3), and renders Write1:
- "error": the Read is on its default "On missing frame: Error". The render must fail
  ("Error caught while rendering") and write no frame: it wrote three black frames and
  reported a successful render;
- "black": "On missing frame: Black". The render must write the three (black) frames.

Exit status: 0 on success, 77 if skipped (in CI unless NATRON_GUI_TESTS=1, without OCIO, or if the
Read/Write PNG plug-ins of openfx-io are not available: set OFX_PLUGIN_PATH), 1 on failure.
--debugger runs NatronRenderer under natron-lldb.sh (see run_gui_smoke_test.py), or set
NATRON_TEST_WRAPPER.
"""

import os
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from run_gui_smoke_test import killTree  # noqa: E402

SKIP = 77
TIMEOUT = 300  # seconds per run
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
else:
    read = app.createReader(os.path.join(OUT, "missing", "plate.####.png"))
    onMissingFrame = read.getParam("onMissingFrame")
    onMissingFrame.setValue(onMissingFrame.getOptions().index(MODE))
    write = app.createWriter(os.path.join(OUT, MODE + "_####.png"))
    write.setScriptName("Write1")
    write.connectInput(0, read)
    write.getParam("frameRange").setValue(2)  # manual
    write.getParam("firstFrame").setValue(@FIRST@)
    write.getParam("lastFrame").setValue(@LAST@)
""".replace("@FIRST@", str(FRAMES[0])).replace("@LAST@", str(FRAMES[-1]))


def runMode(renderer, wrapper, outDir, script, mode):
    """Runs the script with the given mode: returns True on success, SKIP if skipped, False on failure."""
    env = dict(os.environ)
    env["NATRON_MISSING_READ_TEST_DIR"] = outDir
    env["NATRON_MISSING_READ_TEST_MODE"] = mode
    # Debug builds: no trapped floating point exception (#1537), as with Natron --test
    env["NATRON_NO_FP_TRAPS"] = "1"
    cmd = wrapper + [renderer, "--no-settings", "-b", script]
    print("Running (%s): %s" % (mode, " ".join(cmd)))
    sys.stdout.flush()
    # in its own process group, for killTree()
    proc = subprocess.Popen(cmd, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            start_new_session=(os.name == "posix"))
    try:
        outBytes, _ = proc.communicate(timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        killTree(proc)
        outBytes, _ = proc.communicate()
        sys.stdout.write(outBytes.decode(errors="replace"))
        print("FAILED (%s): timeout after %d s" % (mode, TIMEOUT))
        return False
    out = outBytes.decode(errors="replace")
    sys.stdout.write(out)
    if "SKIP:" in out:
        return SKIP
    # the exit status is not checked: a failed render gives the status 0 (#1599)
    print("NatronRenderer -b (%s) exited with status %d" % (mode, proc.returncode))
    written = sorted(f for f in os.listdir(outDir) if f.startswith(mode + "_") and f.endswith(".png"))
    failed = "Error caught while rendering" in out
    ok = True
    if mode == "error":
        if written:
            print("FAILED (%s): the render wrote %s, with an input that failed" % (mode, " ".join(written)))
            ok = False
        if not failed:
            print("FAILED (%s): the render did not report a failure" % mode)
            ok = False
    else:
        expected = ["%s_%04d.png" % (mode, frame) for frame in FRAMES]
        if written != expected:
            print("FAILED (%s): the render wrote [%s] instead of [%s]" % (mode, " ".join(written), " ".join(expected)))
            ok = False
        if failed:
            print("FAILED (%s): the render reported a failure" % mode)
            ok = False
    return ok


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    if os.environ.get("CI") and os.environ.get("NATRON_GUI_TESTS") != "1":
        print("renderer missing Read test skipped in CI (set NATRON_GUI_TESTS=1 to run it, as for GuiSmokeTest)")
        return SKIP
    if not os.environ.get("OCIO"):
        # the Write node fails without an OCIO config ("Invalid OCIO config"), as for the Tests binary
        print("renderer missing Read test skipped: set OCIO to an OpenColorIO config")
        return SKIP
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    outDir = tempfile.mkdtemp(prefix="natron-missing-read-test-")
    # a private disk cache (and OFX plug-in cache), removed with outDir, unless NATRON_DISK_CACHE_PATH is set
    if not os.environ.get("NATRON_DISK_CACHE_PATH"):
        os.environ["NATRON_DISK_CACHE_PATH"] = os.path.join(outDir, "cache")
    try:
        script = os.path.join(outDir, "missing_read_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        ok = True
        for mode in ("error", "black"):
            ret = runMode(argv[1], wrapper, outDir, script, mode)
            if ret == SKIP:
                print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
                return SKIP
            if not ret:
                ok = False
        if ok:
            print("renderer missing Read test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
