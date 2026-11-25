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

Then NatronRenderer -t runs the commands of its prompt from stdin: the afterNodeCreated callback
of the project calls natron.quit(4) when the prompt creates a node. The SystemExit raised in the
callback must end the prompt once the callback has returned (the commands after it must not
run), and the exit status must be 4.

Exit status: 0 on success, 77 if the render part is skipped (the Read/Write PNG plug-ins of openfx-io
are not available: set OFX_PLUGIN_PATH), 1 on failure. --debugger runs NatronRenderer under
natron-lldb.sh, or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

from natronrunner import SKIP, natron_env, run, wrapper, write_png

TIMEOUT = 300  # seconds, for each run of NatronRenderer
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


PROMPT_SETUP = """
from NatronEngine import natron

def quitOnNodeCreated(thisNode, app, userEdited):
    natron.quit(4)

import __main__
__main__.quitOnNodeCreated = quitOnNodeCreated
app1.getProjectParam("afterNodeCreated").set("quitOnNodeCreated")
"""

# the commands typed at the -t prompt
PROMPT_STDIN = """
import sys, time
sys.stdout.write("prompt started\\n"); sys.stdout.flush()
app.createNode("fr.inria.built-in.Dot")
for i in range(20):
    time.sleep(0.05)
sys.stdout.write("prompt still running after natron.quit()\\n"); sys.stdout.flush()
"""


def run_prompt_quit(renderer, cmd_wrapper, out_dir):
    """natron.quit() in a callback while the -t prompt runs (#1292): returns True on success."""
    setup = os.path.join(out_dir, "prompt_setup.py")
    with open(setup, "w") as f:
        f.write(PROMPT_SETUP)
    stdin_path = os.path.join(out_dir, "prompt_stdin.py")
    with open(stdin_path, "w") as f:
        f.write(PROMPT_STDIN)
    status, out = run(cmd_wrapper + [renderer, "--no-settings", "-t", setup], natron_env(out_dir), TIMEOUT,
                      stdin=stdin_path)
    sys.stdout.write(out)
    if status is None:
        print("FAILED: timeout (-t prompt)")
        return False
    print("NatronRenderer -t exited with status %d" % status)
    ok = True
    if "prompt started" not in out:
        print("FAILED: the commands of the -t prompt did not run")
        ok = False
    if "prompt still running" in out:
        print("FAILED: natron.quit() in a callback did not end the -t prompt")
        ok = False
    if status != 4:
        print("FAILED: the exit status of the -t prompt should be 4, given to natron.quit()")
        ok = False
    return ok


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    cmd_wrapper = wrapper(argv[2:])
    out_dir = tempfile.mkdtemp(prefix="natron-quit-test-")
    try:
        write_png(os.path.join(out_dir, "input.png"), 64, 48)
        script = os.path.join(out_dir, "quit_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        env = natron_env(out_dir, NATRON_QUIT_TEST_DIR=out_dir)
        status, out = run(cmd_wrapper + [argv[1], "--no-settings", "-w", "Write1", "1-%d" % LAST_FRAME, script],
                          env, TIMEOUT)
        sys.stdout.write(out)
        if status is None:
            print("FAILED: timeout")
            return 1
        if "SKIP:" in out:
            print("render part skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to "
                  "openfx-io); the -t prompt part needs no plug-in")
            return SKIP if run_prompt_quit(argv[1], cmd_wrapper, out_dir) else 1
        frames = [f for f in os.listdir(out_dir) if f.startswith("out_") and f.endswith(".png")]
        print("NatronRenderer exited with status %d after rendering %d of %d frames" % (status, len(frames), LAST_FRAME))
        ok = True
        if status != 3:
            print("FAILED: the exit status should be 3, given to natron.quit()")
            ok = False
        if "afterFrameRender" not in out:
            print("FAILED: the afterFrameRender callback did not run")
            ok = False
        if len(frames) > MAX_FRAMES:
            print("FAILED: natron.quit() in afterFrameRender should stop the render (at most %d frames)" % MAX_FRAMES)
            ok = False
        if not run_prompt_quit(argv[1], cmd_wrapper, out_dir):
            ok = False
        if ok:
            print("renderer quit test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
