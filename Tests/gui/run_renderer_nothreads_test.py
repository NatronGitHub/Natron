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

"""Headless test of NatronRenderer with multithreading off (the noRenderThreads=-1 setting, #2019).

Usage: run_renderer_nothreads_test.py /path/to/NatronRenderer [--debugger]

With noRenderThreads=-1, a blocking render returned before the end of the render, and its request
stayed queued on the writer: the next render of the same writer was queued behind it, and the end of
the render notified the destroyed blocking render, which crashed NatronRenderer.

- NatronRenderer --setting noRenderThreads=-1 -t script.py: the script renders Read -> Write1 with
  app.render() twice (frames 1-3, then 4-6). Each app.render() must return once its frames are
  written. The script saves the project, and NatronRenderer exits with status 0 at the end of stdin.
- NatronRenderer --setting noRenderThreads=-1 -w Write1 7-9 project.ntp: the frames 7-9 are written,
  exit status 0.

Exit status: 0 on success, 77 if skipped (the Read/Write PNG plug-ins of openfx-io are not
available: set OFX_PLUGIN_PATH), 1 on failure. --debugger runs NatronRenderer under natron-lldb.sh,
or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

import natronrunner

TIMEOUT = 120  # seconds for one run of NatronRenderer
SETTINGS = ["--setting", "noRenderThreads=-1"]

SCRIPT = """
import os
import sys
from NatronEngine import natron
OUT = os.environ["NATRON_NOTHREADS_TEST_DIR"]


def written(first, last):
    return [f for f in range(first, last + 1) if os.path.exists(os.path.join(OUT, "out_%03d.png" % f))]


ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n")
    sys.stdout.flush()
    natron.quit(77)
else:
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(OUT + "/input.png")
    write = app.createNode("fr.inria.built-in.Write")
    write.setScriptName("Write1")
    write.connectInput(0, read)
    write.getParam("filename").set(OUT + "/out_###.png")
    for first, last in ((1, 3), (4, 6)):
        app.render(write, first, last)
        # the render is blocking: its frames are written when app.render() returns
        sys.stdout.write("rendered %d-%d: %d frames written\\n" % (first, last, len(written(first, last))))
        sys.stdout.flush()
    app.saveProjectAs(OUT + "/project.ntp")
    sys.stdout.write("script done\\n")
    sys.stdout.flush()
"""


def count_frames(out_dir, first, last):
    """The number of frames among first..last that Write1 wrote in out_dir."""
    return len([f for f in range(first, last + 1) if os.path.exists(os.path.join(out_dir, "out_%03d.png" % f))])


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    renderer = natronrunner.wrapper(argv[2:]) + [argv[1], "--no-settings"] + SETTINGS
    out_dir = tempfile.mkdtemp(prefix="natron-nothreads-test-")
    try:
        natronrunner.write_png(os.path.join(out_dir, "input.png"), 64, 48)
        script = os.path.join(out_dir, "nothreads_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        env = natronrunner.natron_env(out_dir, NATRON_NOTHREADS_TEST_DIR=out_dir.replace("\\", "/"))

        # -t: the script renders twice with app.render(), then the prompt reads the null device and exits
        status, out = natronrunner.run(renderer + ["-t", script], env, TIMEOUT)
        sys.stdout.write(out)
        if "SKIP:" in out:
            print("skipped: the PNG Read/Write plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io)")
            return natronrunner.SKIP
        ok = True
        if status != 0:
            print("FAILED: NatronRenderer -t exited with status %s instead of 0" % status)
            ok = False
        for first, last in ((1, 3), (4, 6)):
            if "rendered %d-%d: 3 frames written" % (first, last) not in out:
                print("FAILED: app.render(write, %d, %d) returned before its 3 frames were written" % (first, last))
                ok = False
        if "script done" not in out:
            print("FAILED: the script did not run to its end")
            ok = False
        if count_frames(out_dir, 1, 6) != 6:
            print("FAILED: %d of the frames 1-6 were written instead of 6" % count_frames(out_dir, 1, 6))
            ok = False

        # the project saved by the script, rendered with -w
        project = os.path.join(out_dir, "project.ntp")
        if os.path.exists(project):
            status, out = natronrunner.run(renderer + ["-w", "Write1", "7-9", project], env, TIMEOUT)
            sys.stdout.write(out)
            if status != 0:
                print("FAILED: NatronRenderer -w Write1 7-9 exited with status %s instead of 0" % status)
                ok = False
            if count_frames(out_dir, 7, 9) != 3:
                print("FAILED: %d of the frames 7-9 were written instead of 3" % count_frames(out_dir, 7, 9))
                ok = False
        else:
            print("FAILED: the script did not save the project")
            ok = False
        if ok:
            print("renderer nothreads test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
