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

"""Headless test of renders started from a NatronRenderer -t script, which run Python (#1568).

Usage: run_renderer_callbacks_test.py /path/to/NatronRenderer [--debugger]

NatronRenderer -t runs a script that creates Read -> Write1 (frames 1-3), sets the beforeRender
and afterFrameRender callbacks of Write1, then starts the render:
- "button": by triggering the Render button of the Write node (startRender.trigger()). The main
  thread waits for the render, which runs Python on other threads: it deadlocked when the main
  thread kept the Python GIL while waiting (#1568);
- "apprender": with app.render(Write1, 1, 3);
- "expression": the Render button, without callbacks, but with a Python expression on a parameter
  of the Write node that the render reads at each frame;
- "precomp": a first run ("precompsave") saves the project, a second run loads it in a Precomp
  node, chooses Write1 and triggers the Render button of the Precomp node.
Each run must finish under a timeout, run the callbacks (beforeRender once, afterFrameRender
for each frame) or the expression, write the 3 frames, and run the rest of the script after the render.

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
from run_gui_smoke_test import killTree, writePNG  # noqa: E402

SKIP = 77
TIMEOUT = 300  # seconds per run; a deadlock never ends
FRAMES = [1, 2, 3]

SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_CALLBACKS_TEST_DIR"]
MODE = os.environ["NATRON_CALLBACKS_TEST_MODE"]
# natron.quit() does not stop the script: the modes are functions


def beforeRender(thisNode, app):
    sys.stdout.write("beforeRender %s\\n" % thisNode.getScriptName()); sys.stdout.flush()


def afterFrame(frame, thisNode, app):
    sys.stdout.write("afterFrameRender %d\\n" % frame); sys.stdout.flush()


import __main__
__main__.beforeRender = beforeRender
__main__.afterFrame = afterFrame
__main__.exprEvals = []  # (frame, on the main thread) of each evaluation of the expression


def printWritten():
    prefix = MODE.replace("save", "") + "_"
    written = sorted(f for f in os.listdir(OUT) if f.startswith(prefix) and f.endswith(".png"))
    sys.stdout.write("render returned, frames written: %s\\n" % " ".join(written)); sys.stdout.flush()


def renderPrecomp():
    # the project saved by the "precompsave" run, with Write1 and its callbacks
    precomp = app.createNode("fr.inria.built-in.Precomp")
    precomp.getParam("projectFilename").set(os.path.join(OUT, "precomp_sub.ntp"))
    writeNode = precomp.getParam("writeNode")
    writeNode.set(writeNode.getOptions().index("Write1"))  # exactly Write1, not Write1.internalEncoderNode
    precomp.getParam("render").trigger()
    printWritten()


def renderWrite():
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(os.path.join(OUT, "input.png"))
    write = app.createNode("fr.inria.built-in.Write")
    write.setScriptName("Write1")
    write.connectInput(0, read)
    write.getParam("filename").set(os.path.join(OUT, MODE.replace("save", "") + "_###.png"))
    if MODE == "expression":
        # read by the PNG writer at each frame, on the render threads
        write.getParam("compressionLevel").setExpression(
            "import __main__, threading\\n"
            "__main__.exprEvals.append((int(frame), threading.current_thread() is threading.main_thread()))\\n"
            "ret = 6", True)
    else:
        write.getParam("beforeRender").set("beforeRender")
        write.getParam("afterFrameRender").set("afterFrame")
    write.getParam("frameRange").set(2)  # manual
    write.getParam("firstFrame").set(@FIRST@)
    write.getParam("lastFrame").set(@LAST@)
    if MODE == "precompsave":
        app.saveProjectAs(os.path.join(OUT, "precomp_sub.ntp"))
        sys.stdout.write("project saved\\n"); sys.stdout.flush()
        return
    if MODE in ("button", "expression"):
        write.getParam("startRender").trigger()
    else:
        app.render(write, @FIRST@, @LAST@)
    printWritten()
    if MODE == "expression":
        evals = __main__.exprEvals
        sys.stdout.write("expression evaluated %d times, %d off the main thread\\n" % (len(evals), len([e for e in evals if not e[1]]))); sys.stdout.flush()


ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
elif MODE == "precomp":
    renderPrecomp()
else:
    renderWrite()
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
        # in its own process group, for killTree()
        proc = subprocess.Popen(cmd, env=env, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                start_new_session=(os.name == "posix"))
        try:
            outBytes, _ = proc.communicate(timeout=TIMEOUT)
        except subprocess.TimeoutExpired:
            # also kills NatronRenderer under natron-lldb.sh, which runs it in another process group
            killTree(proc)
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
    if mode == "precompsave":
        if "project saved" not in lines:
            print("FAILED (%s): the project was not saved" % mode)
            ok = False
        return ok
    if mode == "expression":
        if not any(l.startswith("expression evaluated ") and not l.startswith("expression evaluated 0 ") for l in lines):
            print("FAILED (%s): the render did not evaluate the expression" % mode)
            ok = False
    else:
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
    # a private disk cache (and OFX plug-in cache), removed with outDir, unless NATRON_DISK_CACHE_PATH is set:
    # the user's one is shared by every Natron of this version, e.g. a concurrent test run (#1569)
    if not os.environ.get("NATRON_DISK_CACHE_PATH"):
        os.environ["NATRON_DISK_CACHE_PATH"] = os.path.join(outDir, "cache")
    try:
        writePNG(os.path.join(outDir, "input.png"), 64, 48)
        script = os.path.join(outDir, "callbacks_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        ok = True
        for mode in ("button", "apprender", "expression", "precompsave", "precomp"):
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
