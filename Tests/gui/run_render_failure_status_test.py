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

Usage: run_render_failure_status_test.py /path/to/NatronRenderer [--debugger]

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
- WriteBad rendered with app.render() by a script run with -t (the interpreter then reads the end of
  its stdin): "Error while rendering" in the output, exit status 1.

Renders that wrote nothing, or not every frame, with exit status 0 or never finished (#817):
- WriteGood whose "Before frame render" callback has the wrong signature: the render never
  finished. Now: "Wrong signature" in the output, no frame, exit status 1.
- The same with the "After frame render" callback: exit status 1. The render is aborted after the
  first frame, so WriteGood writes 1 to 3 frames, depending on the render threads.
- -w WriteGood 3-1 (first frame after the last frame): the render never finished. Now: exit status 1.
- WriteGood with the frames 3 to 1 in its own frame range: nothing written, exit status 0. Now: 1.
- -w WriteGood -w WriteOff, WriteOff being disabled: WriteOff was skipped, exit status 0. Now: 1.
- WriteNoInput, a Write node with no input: it wrote 3 black frames, exit status 0. Now, with
  -w WriteNoInput: nothing, exit status 1. When NatronRenderer renders all the Write nodes (no -w),
  WriteNoInput is skipped with a warning, as a disabled one is, and WriteGood writes its 3 frames;
  without another Write node, that message is the error, exit status 1.

Exit status: 0 on success, 77 if skipped (the Read/Write PNG plug-ins of openfx-io are not
available: set OFX_PLUGIN_PATH), 1 on failure. --debugger runs NatronRenderer under natron-lldb.sh,
or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import sys
import tempfile

import natronrunner

SKIP_CASE = 78
TIMEOUT = 90  # seconds for one run; 15 runs (plus 10 s to kill each) stay below the ctest timeout (1600)
ERROR_TEXT = "Error while rendering"
# printed when the sequence render of a writer cannot begin (a movie writer that cannot open its file)
BEGIN_ERROR_TEXT = "could not start the render of the sequence"
RANGE_ERROR_TEXT = "First frame index in the sequence is greater than the last frame index"

SCRIPT = """
import __main__
import os
import sys
from NatronEngine import natron
OUT = os.environ["NATRON_FAILURE_TEST_DIR"]
WRITERS = os.environ["NATRON_FAILURE_TEST_WRITERS"].split(",")
FILES = {"WriteGood": "frames/out_###.png", "WriteBad": "blocked/out_###.png", "WriteBadMovie": "blocked/out.mov",
         "WriteOff": "frames/off_###.png", "WriteNoInput": "frames/out_###.png"}
CALLBACK = os.environ.get("NATRON_FAILURE_TEST_CALLBACK", "")  # beforeFrameRender or afterFrameRender


def wrong_signature(frame):
    pass


# the callbacks are looked up in the main module
__main__.wrong_signature = wrong_signature

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
        write.disconnectInput(0)
        if name != "WriteNoInput":
            write.connectInput(0, read)
        write.getParam("filename").set(OUT + "/" + FILES[name])
        write.getParam("frameRange").set(2)  # manual
        reversed_range = os.environ.get("NATRON_FAILURE_TEST_REVERSED")
        write.getParam("firstFrame").set(3 if reversed_range else 1)
        write.getParam("lastFrame").set(1 if reversed_range else 3)
        if name == "WriteOff":
            write.getParam("disableNode").set(True)
        if CALLBACK:
            write.getParam(CALLBACK).set("wrong_signature")
        if os.environ.get("NATRON_FAILURE_TEST_QUIT") or os.environ.get("NATRON_FAILURE_TEST_RENDER"):
            app.render(write, 1, 3)
    if os.environ.get("NATRON_FAILURE_TEST_QUIT"):
        natron.quit(0)
"""


def render_case(renderer, tmp, case, writers, renderer_args=(), quit_after=False, **variables):
    """Runs NatronRenderer on a project with the given Write nodes.

    Args:
        renderer: The command that runs NatronRenderer (with its wrapper).
        tmp: Directory in which the directory of this case is created.
        case: Name of the case, used for its directory.
        writers: Script names of the Write nodes to create, among WriteGood, WriteBad and
            WriteBadMovie.
        renderer_args: Extra command-line arguments, before the script.
        quit_after: If true, the script renders the writers itself with app.render(), then calls
            natron.quit(0).
        **variables: More environment variables for the script: NATRON_FAILURE_TEST_RENDER (it renders
            the writers with app.render()), NATRON_FAILURE_TEST_CALLBACK (the name of the callback
            parameter that gets a function with the wrong signature), NATRON_FAILURE_TEST_REVERSED (the
            frame range of the writers is 3 to 1).

    Returns:
        A tuple (status, output, frames): the exit status (None after a timeout), the text printed
        by NatronRenderer, and the number of frames written by WriteGood.
    """
    out = os.path.join(tmp, case)
    os.makedirs(os.path.join(out, "frames"))
    natronrunner.write_png(os.path.join(out, "input.png"))
    # a regular file where the output directory of WriteBad should be
    with open(os.path.join(out, "blocked"), "w") as f:
        f.write("not a directory")
    script = os.path.join(out, "script.py")
    with open(script, "w") as f:
        f.write(SCRIPT)
    # FP traps on, as before (the other drivers disable them, as Natron --test does)
    env = natronrunner.natron_env(tmp, NATRON_FAILURE_TEST_DIR=out.replace("\\", "/"),
                                  NATRON_FAILURE_TEST_WRITERS=",".join(writers),
                                  NATRON_NO_FP_TRAPS=os.environ.get("NATRON_NO_FP_TRAPS"))
    for name in ("NATRON_FAILURE_TEST_QUIT", "NATRON_FAILURE_TEST_RENDER", "NATRON_FAILURE_TEST_CALLBACK",
                 "NATRON_FAILURE_TEST_REVERSED"):
        env.pop(name, None)
    if quit_after:
        env["NATRON_FAILURE_TEST_QUIT"] = "1"
    env.update(variables)
    status, output = natronrunner.run(renderer + ["--no-settings"] + list(renderer_args) + [script], env, TIMEOUT)
    frames = len([f for f in os.listdir(os.path.join(out, "frames")) if f.startswith("out_")])
    return status, output, frames


def check(what, result, status, frames, error_text, also_text=None):
    """Compares the result of render_case() with what is expected.

    Args:
        what: Description of the case, for the message.
        result: The tuple returned by render_case().
        status: Expected exit status.
        frames: Expected number of frames written by WriteGood, or the range of the expected numbers.
        error_text: Whether ERROR_TEXT must be in the output (True) or must not (False).
        also_text: Another text that must be in the output, or None.

    Returns:
        True if the result is as expected. If not, the reason is printed.
    """
    got_status, output, got_frames = result
    problems = []
    if got_status != status:
        problems.append("exit status %s instead of %d" % (got_status, status))
    expected_frames = frames if isinstance(frames, range) else range(frames, frames + 1)
    if got_frames not in expected_frames:
        problems.append("%d frames written by WriteGood instead of %s" % (got_frames, frames))
    if (ERROR_TEXT in output) != error_text:
        problems.append('"%s" %s the output' % (ERROR_TEXT, "is not in" if error_text else "is in"))
    if also_text is not None and also_text not in output:
        problems.append('"%s" is not in the output' % also_text)
    if problems:
        print("FAILED: %s: %s\n%s" % (what, ", ".join(problems), output[-3000:]))
    return not problems


def main():
    """Runs every case; returns 0 if all pass, 1 on failure, SKIP without plug-ins."""
    if len(sys.argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    renderer = natronrunner.wrapper(sys.argv[2:]) + [sys.argv[1]]
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        result = render_case(renderer, tmp, "good", ["WriteGood"])
        if result[0] == natronrunner.SKIP:
            print("skipped: no PNG reader or writer (set OFX_PLUGIN_PATH)")
            return natronrunner.SKIP
        ok &= check("a render to an existing directory", result, 0, 3, False)
        ok &= check("a render to a path that cannot be created",
                    render_case(renderer, tmp, "bad", ["WriteBad"]), 1, 0, True)
        ok &= check("two Write nodes, one failing",
                    render_case(renderer, tmp, "both", ["WriteGood", "WriteBad"]), 1, 3, True)
        ok &= check("two Write nodes, one failing, -w with the other one",
                    render_case(renderer, tmp, "selected", ["WriteGood", "WriteBad"], ["-w", "WriteGood"]), 0, 3, False)
        ok &= check("natron.quit(0) after a failed app.render()",
                    render_case(renderer, tmp, "quit", ["WriteBad"], quit_after=True), 0, 0, True)
        result = render_case(renderer, tmp, "movie", ["WriteBadMovie"])
        if result[0] == SKIP_CASE:
            print("no movie writer: the failure at the beginning of a sequence render was not tested")
        else:
            ok &= check("a movie writer that cannot open its file", result, 1, 0, True, BEGIN_ERROR_TEXT)
        ok &= check("app.render() of a failing writer in a script run with -t",
                    render_case(renderer, tmp, "interpreter", ["WriteBad"], ["-t"], NATRON_FAILURE_TEST_RENDER="1"),
                    1, 0, True)
        # renders that wrote nothing, or not every frame, with exit status 0, or never finished (#817)
        ok &= check("a beforeFrameRender callback with the wrong signature",
                    render_case(renderer, tmp, "before", ["WriteGood"], NATRON_FAILURE_TEST_CALLBACK="beforeFrameRender"),
                    1, 0, False, "Wrong signature of beforeFrameRendered callback")
        ok &= check("an afterFrameRender callback with the wrong signature",
                    render_case(renderer, tmp, "after", ["WriteGood"], NATRON_FAILURE_TEST_CALLBACK="afterFrameRender"),
                    1, range(1, 4), False, "Wrong signature of onFrameRendered callback")
        ok &= check("-w WriteGood 3-1",
                    render_case(renderer, tmp, "reversed_cl", ["WriteGood"], ["-w", "WriteGood", "3-1"]),
                    1, 0, False, RANGE_ERROR_TEXT)
        ok &= check("a Write node whose frame range is 3 to 1",
                    render_case(renderer, tmp, "reversed", ["WriteGood"], NATRON_FAILURE_TEST_REVERSED="1"),
                    1, 0, False, RANGE_ERROR_TEXT)
        ok &= check("-w with a disabled Write node",
                    render_case(renderer, tmp, "disabled", ["WriteGood", "WriteOff"], ["-w", "WriteGood", "-w", "WriteOff"]),
                    1, 3, False, "This node is disabled")
        ok &= check("-w with a Write node with no input",
                    render_case(renderer, tmp, "noinput", ["WriteNoInput"], ["-w", "WriteNoInput"]),
                    1, 0, False, "This node has no input")
        ok &= check("all the Write nodes, all with no input",
                    render_case(renderer, tmp, "noinput_only", ["WriteNoInput"]),
                    1, 0, False, "These Write nodes have no input and are not rendered: WriteNoInput")
        ok &= check("all the Write nodes, one with no input",
                    render_case(renderer, tmp, "noinput_all", ["WriteGood", "WriteNoInput"]),
                    0, 3, False, "These Write nodes have no input and are not rendered: WriteNoInput")
    if ok:
        print("render failure status test passed")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
