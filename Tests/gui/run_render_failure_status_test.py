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

NatronRenderer runs a script that builds a Read and one or more Write nodes on frames 1-3, then
renders them. WriteGood writes to a directory that exists. WriteBad writes to a path that cannot
be created (its directory is a regular file), so its render fails.

- WriteGood alone: 3 frames, exit status 0.
- WriteBad alone: no frame, "Error while rendering" in the output, exit status 1. It was 0, so
  that a render farm manager took the failed render for a success.
- WriteGood and WriteBad: exit status 1, and WriteGood still writes its 3 frames.
- The same project with -w WriteGood: only WriteGood is rendered, exit status 0.
- WriteBad rendered by the script with app.render(), then natron.quit(0): exit status 0.
- WriteBad to a movie file: the writer fails when the sequence render begins, before any frame.
  "Error while rendering" in the output, exit status 1. Skipped without the WriteFFmpeg plug-in.

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
SKIP_CASE = 78
TIMEOUT = 100  # seconds for one run; six runs stay below the ctest timeout (660)
ERROR_TEXT = "Error while rendering"
# printed when the sequence render of a writer cannot begin (a movie writer that cannot open its file)
BEGIN_ERROR_TEXT = "could not start the render of the sequence"

SCRIPT = """
import os, sys
from NatronEngine import natron
OUT = os.environ["NATRON_FAILURE_TEST_DIR"]
WRITERS = os.environ["NATRON_FAILURE_TEST_WRITERS"].split(",")
FILES = {"WriteGood": "frames/out_###.png", "WriteBad": "blocked/out_###.png", "WriteBadMovie": "blocked/out.mov"}
ids = natron.getPluginIDs()
if not any(i.startswith("fr.inria.openfx.ReadPNG") for i in ids) or not any(i.startswith("fr.inria.openfx.WritePNG") for i in ids):
    sys.stdout.write("SKIP: no PNG reader or writer\\n"); sys.stdout.flush()
    natron.quit(77)
elif "WriteBadMovie" in WRITERS and not any(i.startswith("fr.inria.openfx.WriteFFmpeg") for i in ids):
    sys.stdout.write("SKIP: no movie writer\\n"); sys.stdout.flush()
    natron.quit(78)
else:
    read = app.createNode("fr.inria.built-in.Read")
    read.getParam("filename").set(OUT + "/input.png")
    for name in WRITERS:
        write = app.createNode("fr.inria.built-in.Write")
        write.setScriptName(name)
        write.connectInput(0, read)
        write.getParam("filename").set(OUT + "/" + FILES[name])
        write.getParam("frameRange").set(2)  # manual
        write.getParam("firstFrame").set(1)
        write.getParam("lastFrame").set(3)
        if os.environ.get("NATRON_FAILURE_TEST_QUIT"):
            app.render(write, 1, 3)
    if os.environ.get("NATRON_FAILURE_TEST_QUIT"):
        natron.quit(0)
"""


def run(renderer, tmp, case, writers, renderer_args=(), quit_after=False):
    """Runs NatronRenderer on a project with the given Write nodes.

    Args:
        renderer: Path to the NatronRenderer binary.
        tmp: Directory in which the directory of this case is created.
        case: Name of the case, used for its directory.
        writers: Script names of the Write nodes to create, among WriteGood, WriteBad and
            WriteBadMovie.
        renderer_args: Extra command-line arguments, before the script.
        quit_after: If true, the script renders the writers itself with app.render(), then calls
            natron.quit(0).

    Returns:
        A tuple (status, output, frames): the exit status (None after a timeout), the text printed
        by NatronRenderer, and the number of frames written by WriteGood.
    """
    out = os.path.join(tmp, case)
    os.makedirs(os.path.join(out, "frames"))
    writePNG(os.path.join(out, "input.png"))
    # a regular file where the output directory of WriteBad should be
    with open(os.path.join(out, "blocked"), "w") as f:
        f.write("not a directory")
    script = os.path.join(out, "script.py")
    with open(script, "w") as f:
        f.write(SCRIPT)
    env = dict(os.environ, NATRON_FAILURE_TEST_DIR=out.replace("\\", "/"),
               NATRON_FAILURE_TEST_WRITERS=",".join(writers))
    env.pop("NATRON_FAILURE_TEST_QUIT", None)
    if quit_after:
        env["NATRON_FAILURE_TEST_QUIT"] = "1"
    try:
        proc = subprocess.run([renderer, "--no-settings"] + list(renderer_args) + [script], env=env,
                              stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return None, "timeout", 0
    frames = len([f for f in os.listdir(os.path.join(out, "frames")) if f.startswith("out_")])
    return proc.returncode, proc.stdout.decode(errors="replace"), frames


def check(what, result, status, frames, error_text, also_text=None):
    """Compares the result of run() with what is expected.

    Args:
        what: Description of the case, for the message.
        result: The tuple returned by run().
        status: Expected exit status.
        frames: Expected number of frames written by WriteGood.
        error_text: Whether ERROR_TEXT must be in the output (True) or must not (False).
        also_text: Another text that must be in the output, or None.

    Returns:
        True if the result is as expected. If not, the reason is printed.
    """
    got_status, output, got_frames = result
    problems = []
    if got_status != status:
        problems.append("exit status %s instead of %d" % (got_status, status))
    if got_frames != frames:
        problems.append("%d frames written by WriteGood instead of %d" % (got_frames, frames))
    if (ERROR_TEXT in output) != error_text:
        problems.append('"%s" %s the output' % (ERROR_TEXT, "is not in" if error_text else "is in"))
    if also_text is not None and also_text not in output:
        problems.append('"%s" is not in the output' % also_text)
    if problems:
        print("FAILED: %s: %s\n%s" % (what, ", ".join(problems), output[-3000:]))
    return not problems


def main():
    if len(sys.argv) != 2:
        sys.stderr.write(__doc__)
        return 1
    renderer = sys.argv[1]
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        result = run(renderer, tmp, "good", ["WriteGood"])
        if result[0] == SKIP:
            print("skipped: no PNG reader or writer (set OFX_PLUGIN_PATH)")
            return SKIP
        ok &= check("a render to an existing directory", result, 0, 3, False)
        ok &= check("a render to a path that cannot be created",
                    run(renderer, tmp, "bad", ["WriteBad"]), 1, 0, True)
        ok &= check("two Write nodes, one failing",
                    run(renderer, tmp, "both", ["WriteGood", "WriteBad"]), 1, 3, True)
        ok &= check("two Write nodes, one failing, -w with the other one",
                    run(renderer, tmp, "selected", ["WriteGood", "WriteBad"], ["-w", "WriteGood"]), 0, 3, False)
        ok &= check("natron.quit(0) after a failed app.render()",
                    run(renderer, tmp, "quit", ["WriteBad"], quit_after=True), 0, 0, True)
        result = run(renderer, tmp, "movie", ["WriteBadMovie"])
        if result[0] == SKIP_CASE:
            print("no movie writer: the failure at the beginning of a sequence render was not tested")
        else:
            ok &= check("a movie writer that cannot open its file", result, 1, 0, True, BEGIN_ERROR_TEXT)
    if ok:
        print("OK")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
