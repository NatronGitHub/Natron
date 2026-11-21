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

"""Headless test: the OFX plug-ins get kOfxActionUnload once, whatever way a Python script ends (#1557).

Usage: run_renderer_exit_unload_test.py /path/to/NatronRenderer /path/to/test/OfxPlugins [--debugger]

NatronRenderer runs a script that creates a net.natron.test.Sequential node (Tests/OfxTestPlugin), with
NATRON_TEST_PLUGIN_ACTION_LOG set: the test plug-ins write their kOfxActionLoad and kOfxActionUnload to
that file. sys.exit() in the script, or at the prompt of the -t option, leaves through exit(), so
~AppManager never ran and the plug-ins were never unloaded. In each case below, every plug-in that was
loaded must be unloaded exactly once (not twice, by the normal teardown too), with no live instance
left, and the exit status must be the one of the script:
- NatronRenderer script.py, the script ends with sys.exit(3): status 3;
- NatronRenderer -t script.py, sys.exit(5) at the prompt: status 5;
- NatronRenderer -t script.py, the prompt ends (end of stdin): status 0;
- NatronRenderer script.py, the script raises an exception: status 1.

Exit status: 0 on success, 77 if skipped (the test plug-in was not built), 1 on failure.
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

SKIP = 77
PLUGIN_ID = "net.natron.test.Sequential"

SCRIPT = """
import sys
from NatronEngine import natron
node = app.createNode("%s")
sys.stdout.write("node created: %%s\\n" %% (node is not None)); sys.stdout.flush()
""" % PLUGIN_ID

CASES = [
    # (name, script ending, -t, stdin of the -t prompt, expected exit status)
    ("sys.exit() in the script", "sys.exit(3)\n", False, None, 3),
    ("sys.exit() at the -t prompt", "", True, "import sys\nsys.exit(5)\n", 5),
    ("end of the -t prompt", "", True, "", 0),
    ("exception in the script", "raise RuntimeError('exception raised by the test script')\n", False, None, 1),
]


def checkLog(logPath):
    """Returns the error messages: each loaded plug-in must be unloaded once, with no live instance."""
    errors = []
    loads = {}
    unloads = {}
    try:
        with open(logPath) as f:
            lines = f.read().splitlines()
    except OSError:
        lines = []
    for line in lines:
        words = line.split()
        if len(words) >= 2 and words[0] == "load":
            loads[words[1]] = loads.get(words[1], 0) + 1
        elif len(words) >= 3 and words[0] == "unload":
            unloads[words[1]] = unloads.get(words[1], 0) + 1
            if words[2] != "live=0":
                errors.append("%s was unloaded with an instance left (%s)" % (words[1], words[2]))
    seqId = PLUGIN_ID.split(".")[-1]
    if not loads.get(seqId):
        errors.append("%s was never loaded" % PLUGIN_ID)
    for pid in sorted(set(loads) | set(unloads)):
        if loads.get(pid, 0) != unloads.get(pid, 0):
            errors.append("%s: kOfxActionLoad %d time(s), kOfxActionUnload %d time(s)"
                          % (pid, loads.get(pid, 0), unloads.get(pid, 0)))
    return errors


def runCase(renderer, wrapper, pluginDir, outDir, index, case):
    name, ending, interpreter, stdinText, expectedStatus = case
    script = os.path.join(outDir, "exit_unload_%d.py" % index)
    with open(script, "w") as f:
        f.write(SCRIPT + ending)
    logPath = os.path.join(outDir, "actions_%d.log" % index)
    env = dict(os.environ)
    env["OFX_PLUGIN_PATH"] = pluginDir
    env["NATRON_TEST_PLUGIN_ACTION_LOG"] = logPath
    env["NATRON_DISK_CACHE_PATH"] = os.path.join(outDir, "cache")
    # Debug builds: no trapped floating point exception (#1537), as with Natron --test
    env["NATRON_NO_FP_TRAPS"] = "1"
    cmd = wrapper + [renderer, "--no-settings"] + (["-t"] if interpreter else []) + [script]
    stdinPath = os.devnull
    if interpreter:
        stdinPath = os.path.join(outDir, "stdin_%d.py" % index)
        with open(stdinPath, "w") as f:
            f.write(stdinText)
        env["NATRON_LLDB_STDIN"] = stdinPath  # natron-lldb.sh reads stdin from there
    print("=== %s: %s < %s" % (name, " ".join(cmd), stdinPath))
    sys.stdout.flush()
    try:
        with open(stdinPath, "rb") as stdin:
            proc = subprocess.run(cmd, env=env, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  timeout=300)
    except subprocess.TimeoutExpired:
        print("FAILED (%s): timeout" % name)
        return False
    out = proc.stdout.decode(errors="replace")
    sys.stdout.write(out)
    print("NatronRenderer exited with status %d" % proc.returncode)
    ok = True
    if "node created: True" not in out:
        print("FAILED (%s): the script did not create the %s node" % (name, PLUGIN_ID))
        ok = False
    if proc.returncode != expectedStatus:
        print("FAILED (%s): the exit status should be %d" % (name, expectedStatus))
        ok = False
    for error in checkLog(logPath):
        print("FAILED (%s): %s" % (name, error))
        ok = False
    return ok


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(__doc__)
        return 2
    renderer = argv[1]
    pluginDir = argv[2]
    if not os.path.isdir(os.path.join(pluginDir, "NatronTestPlugin.ofx.bundle")):
        print("skipped: the test plug-in is not in %s" % pluginDir)
        return SKIP
    wrapper = []
    if "--debugger" in argv[3:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    outDir = tempfile.mkdtemp(prefix="natron-exit-unload-test-")
    try:
        ok = True
        for index, case in enumerate(CASES):
            if not runCase(renderer, wrapper, pluginDir, outDir, index, case):
                ok = False
        if ok:
            print("renderer exit unload test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
