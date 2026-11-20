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

"""Headless test of natron.quit() called from a render callback.

Usage: run_renderer_quit_test.py /path/to/NatronRenderer [--debugger]

NatronRenderer renders Read -> Write1 on frames 1-200, and the afterFrameRender callback
of Write1 calls natron.quit(3) at the first frame. The render must stop after a few
frames (it rendered all of them when the abort was queued to the main thread, which
waits for the render without running an event loop), and the exit status must be 3.

Exit status: 0 on success, 77 if skipped (in CI unless NATRON_GUI_TESTS=1, or the Read/Write
PNG plug-ins of openfx-io are not available: set OFX_PLUGIN_PATH), 1 on failure.
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
from run_gui_smoke_test import writePNG  # noqa: E402

SKIP = 77
LAST_FRAME = 200
MAX_FRAMES = 30  # rendered frames allowed after natron.quit() at the first one

SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_QUIT_TEST_DIR"]
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
else:
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(os.path.join(OUT, "input.png"))
    write = app.createNode("fr.inria.built-in.Write")
    write.setScriptName("Write1")
    write.connectInput(0, read)
    write.getParam("filename").set(os.path.join(OUT, "out_###.png"))

    def afterFrame(frame, thisNode, app):
        sys.stdout.write("afterFrameRender %d\\n" % frame); sys.stdout.flush()
        natron.quit(3)

    import __main__
    __main__.afterFrame = afterFrame
    write.getParam("afterFrameRender").set("afterFrame")
"""


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    if os.environ.get("CI") and os.environ.get("NATRON_GUI_TESTS") != "1":
        print("renderer quit test skipped in CI (set NATRON_GUI_TESTS=1 to run it, as for GuiSmokeTest)")
        return SKIP
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    outDir = tempfile.mkdtemp(prefix="natron-quit-test-")
    try:
        writePNG(os.path.join(outDir, "input.png"), 64, 48)
        script = os.path.join(outDir, "quit_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        env = dict(os.environ)
        env["NATRON_QUIT_TEST_DIR"] = outDir
        cmd = wrapper + [argv[1], "--no-settings", "-w", "Write1", "1-%d" % LAST_FRAME, script]
        print("Running: " + " ".join(cmd))
        sys.stdout.flush()
        try:
            proc = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
        except subprocess.TimeoutExpired:
            print("FAILED: timeout")
            return 1
        out = proc.stdout.decode(errors="replace")
        sys.stdout.write(out)
        if "SKIP:" in out:
            print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
            return SKIP
        frames = [f for f in os.listdir(outDir) if f.startswith("out_") and f.endswith(".png")]
        print("NatronRenderer exited with status %d after rendering %d of %d frames" % (proc.returncode, len(frames), LAST_FRAME))
        ok = True
        if proc.returncode != 3:
            print("FAILED: the exit status should be 3, given to natron.quit()")
            ok = False
        if "afterFrameRender" not in out:
            print("FAILED: the afterFrameRender callback did not run")
            ok = False
        if len(frames) > MAX_FRAMES:
            print("FAILED: natron.quit() in afterFrameRender should stop the render (at most %d frames)" % MAX_FRAMES)
            ok = False
        if ok:
            print("renderer quit test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
