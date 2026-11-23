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

"""Headless test of the end of the interpreter mode on a stdin that is not a console.

Usage: run_interpreter_stdin_eof_test.py /path/to/NatronRenderer

NatronRenderer -t script.py runs the script, then the Python prompt on stdin. With stdin
redirected from the null device, the prompt must exit at once with status 0. On Windows the
null device passes isatty(), so Python 3.13 and later started its console prompt (pyrepl) on
it: every read failed, the error was printed and the read retried forever, so the process
never exited and its output grew without bound.

With a pipe on stdin, the prompt must run the commands of the pipe, then exit with status 0.

Exit status: 0 on success, 1 on failure.
"""

import os
import subprocess
import sys
import tempfile

TIMEOUT = 120  # seconds, for a run that takes a few seconds
MAX_OUTPUT = 1 << 20  # bytes: the failure printed megabytes of tracebacks per second

SCRIPT = """
import os, sys
if sys.platform == "win32":
    # pyrepl needs _ctypes, whose libffi is next to Natron in an installed build, but only in PATH
    # in a build tree, where Python does not look for the dependencies of its extension modules
    for d in os.environ.get("PATH", "").split(os.pathsep):
        if os.path.isdir(d):
            os.add_dll_directory(d)
sys.stdout.write("SCRIPT_DONE\\n"); sys.stdout.flush()
"""


def run(renderer, script, outPath, stdin, stdinData=None):
    """Runs NatronRenderer -t script with the given stdin, returns (status, output).

    The output goes to a file, not a pipe, and the process is killed when the file grows past
    MAX_OUTPUT or after TIMEOUT: status is then None.
    """
    import time
    with open(outPath, "wb") as out:
        p = subprocess.Popen([renderer, "-t", script], stdin=stdin, stdout=out, stderr=subprocess.STDOUT)
        if stdinData is not None:
            p.stdin.write(stdinData)
            p.stdin.close()
        start = time.time()
        killed = None
        while p.poll() is None:
            time.sleep(0.2)
            if os.path.getsize(outPath) > MAX_OUTPUT:
                killed = "its output grew past %d bytes" % MAX_OUTPUT
            elif time.time() - start > TIMEOUT:
                killed = "it was still running after %d s" % TIMEOUT
            if killed:
                p.kill()
                p.wait()
                break
    with open(outPath, "rb") as f:
        output = f.read(MAX_OUTPUT).decode("utf-8", "replace")
    if killed:
        sys.stdout.write("FAIL: NatronRenderer -t did not exit: %s\n" % killed)
        sys.stdout.write(output[:4000] + "\n")
        return None, output
    return p.returncode, output


def main():
    if len(sys.argv) != 2:
        sys.stderr.write(__doc__)
        return 1
    renderer = sys.argv[1]
    failed = False
    with tempfile.TemporaryDirectory() as tmp:
        script = os.path.join(tmp, "script.py")
        with open(script, "w") as f:
            f.write(SCRIPT)

        # 1. stdin is the null device: the prompt exits at once
        with open(os.devnull, "rb") as devnull:
            status, output = run(renderer, script, os.path.join(tmp, "null.log"), devnull)
        if status != 0 or "SCRIPT_DONE" not in output or "Traceback" in output:
            sys.stdout.write("FAIL: null stdin: status %s\n%s\n" % (status, output[-4000:]))
            failed = True

        # 2. stdin is a pipe: the prompt runs its commands, then exits
        status, output = run(renderer, script, os.path.join(tmp, "pipe.log"), subprocess.PIPE,
                             b'import sys\nsys.stdout.write("PIPED_" + "COMMAND\\n")\n')
        if status != 0 or "SCRIPT_DONE" not in output or "PIPED_COMMAND" not in output or "Traceback" in output:
            sys.stdout.write("FAIL: piped stdin: status %s\n%s\n" % (status, output[-4000:]))
            failed = True

    if not failed:
        sys.stdout.write("OK\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
