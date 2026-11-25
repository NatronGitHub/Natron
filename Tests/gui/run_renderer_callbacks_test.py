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

Exit status: 0 on success, 77 if skipped (without OCIO, or if the Read/Write PNG plug-ins of
openfx-io are not available: set OFX_PLUGIN_PATH), 1 on failure. --debugger runs NatronRenderer
under natron-lldb.sh, or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

import natronrunner

TIMEOUT = 120  # seconds per run (a few seconds; a deadlock never ends), 5 runs within the ctest TIMEOUT
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


def run_mode(renderer, cmd_wrapper, out_dir, script, mode):
    """Runs the script with the given mode: returns True on success, SKIP if skipped, False on failure."""
    env = natronrunner.natron_env(out_dir, NATRON_CALLBACKS_TEST_DIR=out_dir, NATRON_CALLBACKS_TEST_MODE=mode)
    # the -t prompt reads its commands from stdin after the script: none
    stdin_path = os.path.join(out_dir, "empty_stdin.py")
    with open(stdin_path, "w") as f:
        f.write("")
    print("Mode %s:" % mode)
    cmd = cmd_wrapper + [renderer, "--no-settings", "-t", script]
    status, out = natronrunner.run(cmd, env, TIMEOUT, stdin=stdin_path)
    sys.stdout.write(out)
    if status is None:
        print("FAILED (%s): timeout after %d s: deadlock?" % (mode, TIMEOUT))
        return False
    if "SKIP:" in out:
        return natronrunner.SKIP
    print("NatronRenderer -t (%s) exited with status %d" % (mode, status))
    lines = [line.strip() for line in out.splitlines()]
    ok = True
    if status != 0:
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
    if not os.environ.get("OCIO"):
        # the Write node fails without an OCIO config ("Invalid OCIO config"), as for the Tests binary
        print("renderer callbacks test skipped: set OCIO to an OpenColorIO config")
        return natronrunner.SKIP
    cmd_wrapper = natronrunner.wrapper(argv[2:])
    out_dir = tempfile.mkdtemp(prefix="natron-callbacks-test-")
    try:
        natronrunner.write_png(os.path.join(out_dir, "input.png"), 64, 48)
        script = os.path.join(out_dir, "callbacks_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        ok = True
        for mode in ("button", "apprender", "expression", "precompsave", "precomp"):
            ret = run_mode(argv[1], cmd_wrapper, out_dir, script, mode)
            if ret == natronrunner.SKIP:
                print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
                return natronrunner.SKIP
            if not ret:
                ok = False
        if ok:
            print("renderer callbacks test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
