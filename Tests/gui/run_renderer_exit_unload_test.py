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
- NatronRenderer -t script.py, sys.exit(5) at the prompt: status 5 (from Python 3.13 on, Py_Main() returns
  that status instead of calling exit(): Natron exited with status 0);
- NatronRenderer -t script.py, the prompt ends (end of stdin): status 0;
- NatronRenderer -t script.py, a command of the prompt raises an exception: status 1, as with python;
- NatronRenderer script.py, the script raises an exception: status 1.
The commands of the prompt print "prompt ran", which the test checks, so that a prompt that reads nothing
from its redirected stdin is not taken for a success.

Exit status: 0 on success, 1 on failure (also when the test plug-in is not in /path/to/test/OfxPlugins).
--debugger runs NatronRenderer under natron-lldb.sh, or set NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

import natronrunner

PLUGIN_ID = "net.natron.test.Sequential"
CASE_TIMEOUT = 100  # seconds per NatronRenderer run: the 5 runs (plus 10 s grace each) must fit in the ctest TIMEOUT (660 s)

SCRIPT = """
import sys
from NatronEngine import natron
node = app.createNode("%s")
sys.stdout.write("node created: %%s\\n" %% (node is not None)); sys.stdout.flush()
""" % PLUGIN_ID

# the first command typed at the -t prompt
PROMPT_RAN = "import sys; sys.stdout.write('prompt ran\\n'); sys.stdout.flush()\n"

CASES = [
    # (name, script ending, -t, stdin of the -t prompt, expected exit status)
    ("sys.exit() in the script", "sys.exit(3)\n", False, None, 3),
    ("sys.exit() at the -t prompt", "", True, PROMPT_RAN + "import sys\nsys.exit(5)\n", 5),
    ("end of the -t prompt", "", True, PROMPT_RAN, 0),
    ("exception at the -t prompt", "", True, PROMPT_RAN + "raise RuntimeError('exception raised at the prompt')\n", 1),
    ("exception in the script", "raise RuntimeError('exception raised by the test script')\n", False, None, 1),
]


def check_log(log_path):
    """Returns the error messages: each loaded plug-in must be unloaded once, with no live instance."""
    errors = []
    loads = {}
    unloads = {}
    try:
        with open(log_path) as f:
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


def run_case(renderer, cmd_wrapper, plugin_dir, out_dir, index, case):
    name, ending, interpreter, stdin_text, expected_status = case
    script = os.path.join(out_dir, "exit_unload_%d.py" % index)
    with open(script, "w") as f:
        f.write(SCRIPT + ending)
    log_path = os.path.join(out_dir, "actions_%d.log" % index)
    # always a private cache: the OFX plug-in cache affects what the plug-ins get
    env = natronrunner.natron_env(out_dir, OFX_PLUGIN_PATH=plugin_dir, NATRON_TEST_PLUGIN_ACTION_LOG=log_path,
                                  NATRON_DISK_CACHE_PATH=os.path.join(out_dir, "cache"))
    cmd = cmd_wrapper + [renderer, "--no-settings"] + (["-t"] if interpreter else []) + [script]
    stdin_path = None
    if interpreter:
        stdin_path = os.path.join(out_dir, "stdin_%d.py" % index)
        with open(stdin_path, "w") as f:
            f.write(stdin_text)
    print("=== %s" % name)
    status, out = natronrunner.run(cmd, env, CASE_TIMEOUT, stdin=stdin_path)
    sys.stdout.write(out)
    if status is None:
        print("FAILED (%s): timeout after %d s" % (name, CASE_TIMEOUT))
        return False
    print("NatronRenderer exited with status %d" % status)
    ok = True
    if "node created: True" not in out:
        print("FAILED (%s): the script did not create the %s node" % (name, PLUGIN_ID))
        ok = False
    if interpreter and "prompt ran" not in out:
        print("FAILED (%s): the commands of the -t prompt did not run" % name)
        ok = False
    if status != expected_status:
        print("FAILED (%s): the exit status should be %d" % (name, expected_status))
        ok = False
    for error in check_log(log_path):
        print("FAILED (%s): %s" % (name, error))
        ok = False
    return ok


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(__doc__)
        return 2
    renderer = argv[1]
    plugin_dir = argv[2]
    if not os.path.isdir(os.path.join(plugin_dir, "NatronTestPlugin.ofx.bundle")):
        # it is built with the Tests: missing, it is a broken build, not a reason to skip
        print("FAILED: the test plug-in is not in %s" % plugin_dir)
        return 1
    cmd_wrapper = natronrunner.wrapper(argv[3:])
    out_dir = tempfile.mkdtemp(prefix="natron-exit-unload-test-")
    try:
        ok = True
        for index, case in enumerate(CASES):
            if not run_case(renderer, cmd_wrapper, plugin_dir, out_dir, index, case):
                ok = False
        if ok:
            print("renderer exit unload test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
