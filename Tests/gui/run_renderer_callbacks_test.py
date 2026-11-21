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

"""Headless test of renders started from a NatronRenderer -t script, with Python render callbacks (#1568).

Usage: run_renderer_callbacks_test.py /path/to/NatronRenderer [--debugger]

NatronRenderer -t runs a script that sets the beforeRender and afterFrameRender callbacks of a
Write node (Read -> Write1, frames 1-3), then starts the render:
- "button": by triggering the Render button of the Write node (startRender.trigger()). The main
  thread waits for the render, which runs the callbacks on other threads: it deadlocked when the
  main thread kept the Python GIL while waiting (#1568);
- "apprender": with app.render(Write1, 1, 3).
Each run must finish under a timeout, run the callbacks (beforeRender once, afterFrameRender
for each frame), write the 3 frames, and run the rest of the script after the render.

Exit status: 0 on success, 77 if skipped (in CI unless NATRON_GUI_TESTS=1, without OCIO, or if the
Read/Write PNG plug-ins of openfx-io are not available: set OFX_PLUGIN_PATH), 1 on failure.
--debugger runs NatronRenderer under natron-lldb.sh (see run_gui_smoke_test.py), or set
NATRON_TEST_WRAPPER.
"""

import os
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from run_gui_smoke_test import writePNG  # noqa: E402

SKIP = 77
TIMEOUT = 300  # seconds per run; a deadlock never ends
FRAMES = [1, 2, 3]

SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_CALLBACKS_TEST_DIR"]
MODE = os.environ["NATRON_CALLBACKS_TEST_MODE"]
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)

def beforeRender(thisNode, app):
    sys.stdout.write("beforeRender %s\\n" % thisNode.getScriptName()); sys.stdout.flush()

def afterFrame(frame, thisNode, app):
    sys.stdout.write("afterFrameRender %d\\n" % frame); sys.stdout.flush()

import __main__
__main__.beforeRender = beforeRender
__main__.afterFrame = afterFrame

read = app.createNode("fr.inria.built-in.Read")
read.getParam("filename").set(os.path.join(OUT, "input.png"))
write = app.createNode("fr.inria.built-in.Write")
write.setScriptName("Write1")
write.connectInput(0, read)
write.getParam("filename").set(os.path.join(OUT, MODE + "_###.png"))
write.getParam("beforeRender").set("beforeRender")
write.getParam("afterFrameRender").set("afterFrame")
write.getParam("frameRange").set(2)  # manual
write.getParam("firstFrame").set(@FIRST@)
write.getParam("lastFrame").set(@LAST@)
if MODE == "button":
    write.getParam("startRender").trigger()
else:
    app.render(write, @FIRST@, @LAST@)
written = sorted(f for f in os.listdir(OUT) if f.startswith(MODE + "_") and f.endswith(".png"))
sys.stdout.write("render returned, frames written: %s\\n" % " ".join(written)); sys.stdout.flush()
""".replace("@FIRST@", str(FRAMES[0])).replace("@LAST@", str(FRAMES[-1]))


def runMode(renderer, wrapper, outDir, script, mode):
    """Runs the script with the given mode: returns True on success, SKIP if skipped, False on failure."""
    env = dict(os.environ)
    env["NATRON_CALLBACKS_TEST_DIR"] = outDir
    env["NATRON_CALLBACKS_TEST_MODE"] = mode
    # Debug builds: no trapped floating point exception (#1537), as with Natron --test
    env["NATRON_NO_FP_TRAPS"] = "1"
    # the -t prompt reads its commands from stdin after the script: none
    stdinPath = os.path.join(outDir, "empty_stdin.py")
    with open(stdinPath, "w") as f:
        f.write("")
    env["NATRON_LLDB_STDIN"] = stdinPath  # natron-lldb.sh reads stdin from there
    cmd = wrapper + [renderer, "--no-settings", "-t", script]
    print("Running (%s): %s" % (mode, " ".join(cmd)))
    sys.stdout.flush()
    with open(stdinPath, "rb") as stdin:
        # in its own process group, to kill NatronRenderer too if a wrapper (natron-lldb.sh) runs it
        proc = subprocess.Popen(cmd, env=env, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                start_new_session=(os.name == "posix"))
        try:
            outBytes, _ = proc.communicate(timeout=TIMEOUT)
        except subprocess.TimeoutExpired:
            if os.name == "posix":
                os.killpg(proc.pid, signal.SIGKILL)
            else:
                proc.kill()
            outBytes, _ = proc.communicate()
            sys.stdout.write(outBytes.decode(errors="replace"))
            print("FAILED (%s): timeout after %d s: deadlock?" % (mode, TIMEOUT))
            return False
    out = outBytes.decode(errors="replace")
    sys.stdout.write(out)
    if "SKIP:" in out:
        return SKIP
    print("NatronRenderer -t (%s) exited with status %d" % (mode, proc.returncode))
    lines = [line.strip() for line in out.splitlines()]  # "\r\n" under a debugger
    ok = True
    if proc.returncode != 0:
        print("FAILED (%s): the exit status should be 0" % mode)
        ok = False
    if lines.count("beforeRender Write1") != 1:
        print("FAILED (%s): the beforeRender callback should run once" % mode)
        ok = False
    for frame in FRAMES:
        if ("afterFrameRender %d" % frame) not in lines:
            print("FAILED (%s): the afterFrameRender callback did not run for frame %d" % (mode, frame))
            ok = False
    expected = ["%s_%03d.png" % (mode, frame) for frame in FRAMES]
    if ("render returned, frames written: " + " ".join(expected)) not in lines:
        print("FAILED (%s): the render should have written %s when it returned" % (mode, " ".join(expected)))
        ok = False
    return ok


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    if os.environ.get("CI") and os.environ.get("NATRON_GUI_TESTS") != "1":
        print("renderer callbacks test skipped in CI (set NATRON_GUI_TESTS=1 to run it, as for GuiSmokeTest)")
        return SKIP
    if not os.environ.get("OCIO"):
        # the Write node fails without an OCIO config ("Invalid OCIO config"), as for the Tests binary
        print("renderer callbacks test skipped: set OCIO to an OpenColorIO config")
        return SKIP
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    outDir = tempfile.mkdtemp(prefix="natron-callbacks-test-")
    try:
        writePNG(os.path.join(outDir, "input.png"), 64, 48)
        script = os.path.join(outDir, "callbacks_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        ok = True
        for mode in ("button", "apprender"):
            ret = runMode(argv[1], wrapper, outDir, script, mode)
            if ret == SKIP:
                print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
                return SKIP
            if not ret:
                ok = False
        if ok:
            print("renderer callbacks test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
