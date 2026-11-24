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
frames 12 to 14 of the sequence plate.0010.png to plate.0020.png: it read frame 0 ("Cannot load
frame"), or wrote the same frame for every frame. The frames of the sequence are all different
(frame N is N pixels wide), so the frames written must be too.

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
TIMEOUT = 120  # seconds per run; two runs stay below the ctest TIMEOUT (660)
SEQUENCE = range(10, 21)  # the frames of plate.####.png
FRAMES = [12, 13, 14]  # the frames rendered

SAVE_SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_CL_READ_WRITE_TEST_DIR"]
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
else:
    read = app.createNode("fr.inria.built-in.Read")
    read.setScriptName("Read")
    write = app.createNode("fr.inria.built-in.Write")
    write.setScriptName("Write")
    write.connectInput(0, read)
    app.saveProjectAs(os.path.join(OUT, "project.ntp"))
    sys.stdout.write("SAVED\\n"); sys.stdout.flush()
"""


def run(cmd, env):
    """Runs cmd, prints its output, and returns (exit status, output), or None on a timeout."""
    print("Running: %s" % " ".join(cmd))
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
        print("FAILED: timeout after %d s" % TIMEOUT)
        return None
    out = outBytes.decode(errors="replace")
    sys.stdout.write(out)
    print("exit status %d" % proc.returncode)
    return proc.returncode, out


def readFile(path):
    with open(path, "rb") as f:
        return f.read()


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    if os.environ.get("CI") and os.environ.get("NATRON_GUI_TESTS") != "1":
        print("renderer -i/-w test skipped in CI (set NATRON_GUI_TESTS=1 to run it, as for GuiSmokeTest)")
        return SKIP
    if not os.environ.get("OCIO"):
        # the Write node fails without an OCIO config ("Invalid OCIO config"), as for the Tests binary
        print("renderer -i/-w test skipped: set OCIO to an OpenColorIO config")
        return SKIP
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    outDir = tempfile.mkdtemp(prefix="natron-cl-read-write-test-")
    env = dict(os.environ)
    # a private disk cache (and OFX plug-in cache), removed with outDir, unless NATRON_DISK_CACHE_PATH is set
    env.setdefault("NATRON_DISK_CACHE_PATH", os.path.join(outDir, "cache"))
    # Debug builds: no trapped floating point exception (#1537), as with Natron --test
    env["NATRON_NO_FP_TRAPS"] = "1"
    project = os.path.join(outDir, "project.ntp")
    env["NATRON_CL_READ_WRITE_TEST_DIR"] = outDir
    try:
        os.mkdir(os.path.join(outDir, "plate"))
        for frame in SEQUENCE:
            writePNG(os.path.join(outDir, "plate", "plate.%04d.png" % frame), frame, 8)
        script = os.path.join(outDir, "save_project.py")
        with open(script, "w") as f:
            f.write(SAVE_SCRIPT)
        ret = run(wrapper + [argv[1], "--no-settings", "-t", script], env)
        if ret is None:
            return 1
        status, out = ret
        if "SKIP:" in out:
            print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
            return SKIP
        if status != 0 or "SAVED" not in out or not os.path.exists(project):
            print("FAILED: the project was not saved")
            return 1

        ret = run(wrapper + [argv[1], "--no-settings", project,
                             "-i", "Read", os.path.join(outDir, "plate", "plate.####.png"),
                             "-w", "Write", os.path.join(outDir, "out.####.png"),
                             "%d-%d" % (FRAMES[0], FRAMES[-1])], env)
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
        written = sorted(f for f in os.listdir(outDir) if f.startswith("out.") and f.endswith(".png"))
        expected = ["out.%04d.png" % frame for frame in FRAMES]
        if written != expected:
            print("FAILED: the render wrote [%s] instead of [%s]" % (" ".join(written), " ".join(expected)))
            ok = False
        elif len(set(readFile(os.path.join(outDir, f)) for f in written)) != len(written):
            print("FAILED: the frames written are not all different: the same frame of the sequence was read for each")
            ok = False
        if ok:
            print("renderer -i/-w test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
