#!/usr/bin/env python3
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

"""Headless test of App.projectFileHasGuiData() and
App.projectLoadedWithErrors().

Usage: run_project_load_info_test.py /path/to/NatronRenderer [--debugger]

NatronRenderer -t loads, one after the other,
Tests/data/ProjectFileFormat-2.5.ntp (saved in background mode, no error), a
copy of it flagged as saved by the GUI (Background_project=0), and copies with
a node whose plug-in does not exist, at the top level and in a group. It reads
both getters for a new project, after each load and after app.resetProject().

Exit status: 0 on success, 1 on failure, 2 on a usage error.
--debugger runs NatronRenderer under natron-lldb.sh, or set
NATRON_TEST_WRAPPER.
"""

import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.join(os.path.dirname(HERE), "data",
                       "ProjectFileFormat-2.5.ntp")

BACKGROUND_TAG = b"<Background_project>1</Background_project>"
DOT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Dot</Plugin_id>"
# the only Input node of the project is in Group1
INPUT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Input</Plugin_id>"
MISSING_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.NoSuchPlugin</Plugin_id>"

# Run by NatronRenderer -t: writes [label, hasGuiData, loadedWithErrors] for
# each step to NATRON_TEST_OUTPUT (JSON).
SCRIPT = r"""
import json
import os
from NatronEngine import natron

def state(label, a):
    return [label, a.projectFileHasGuiData(), a.projectLoadedWithErrors()]

status = 1
try:
    a = natron.getInstance(0)
    steps = [state("new", a)]
    for label in ("background", "gui", "missingplugin", "missingingroup"):
        loaded = a.loadProject(os.environ["NATRON_TEST_" + label.upper()])
        if loaded is None:
            steps.append([label, "load failed"])
        else:
            steps.append(state(label, loaded))
            a = loaded
    a.resetProject()
    steps.append(state("reset", a))
    with open(os.environ["NATRON_TEST_OUTPUT"], "w") as f:
        json.dump(steps, f)
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
"""

EXPECTED = [
    ["new", False, False],
    ["background", False, False],
    ["gui", True, False],
    ["missingplugin", False, True],
    ["missingingroup", False, True],
    ["reset", False, False],
]


def _write_variant(data, old, new, path):
    """Writes data with the first occurrence of old replaced by new."""
    if old not in data:
        raise ValueError("%s: %r not found in the test project" % (path, old))
    with open(path, "wb") as f:
        f.write(data.replace(old, new, 1))


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    tmp_dir = tempfile.mkdtemp(prefix="natron-project-load-info-test-")
    try:
        with open(PROJECT, "rb") as f:
            data = f.read()
        env = dict(os.environ)
        env["NATRON_DISK_CACHE_PATH"] = os.path.join(tmp_dir, "cache")
        # Debug builds: no trapped floating point exception (#1537)
        env["NATRON_NO_FP_TRAPS"] = "1"
        env["NATRON_TEST_BACKGROUND"] = os.path.join(tmp_dir, "bg.ntp")
        env["NATRON_TEST_GUI"] = os.path.join(tmp_dir, "gui.ntp")
        env["NATRON_TEST_MISSINGPLUGIN"] = os.path.join(tmp_dir, "missing.ntp")
        env["NATRON_TEST_OUTPUT"] = os.path.join(tmp_dir, "out.json")
        shutil.copyfile(PROJECT, env["NATRON_TEST_BACKGROUND"])
        _write_variant(data, BACKGROUND_TAG,
                       b"<Background_project>0</Background_project>",
                       env["NATRON_TEST_GUI"])
        _write_variant(data, DOT_PLUGIN_TAG, MISSING_PLUGIN_TAG,
                       env["NATRON_TEST_MISSINGPLUGIN"])
        env["NATRON_TEST_MISSINGINGROUP"] = os.path.join(tmp_dir,
                                                         "missinggroup.ntp")
        _write_variant(data, INPUT_PLUGIN_TAG, MISSING_PLUGIN_TAG,
                       env["NATRON_TEST_MISSINGINGROUP"])
        script = os.path.join(tmp_dir, "load_info.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        cmd = wrapper + [argv[1], "--no-settings", "-t", script]
        print("Running: " + " ".join(cmd))
        sys.stdout.flush()
        try:
            proc = subprocess.run(cmd, env=env, stdin=subprocess.DEVNULL,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, timeout=300)
        except subprocess.TimeoutExpired:
            print("FAILED: timeout")
            return 1
        sys.stdout.write(proc.stdout.decode(errors="replace"))
        print("exit status %d" % proc.returncode)
        if proc.returncode != 0:
            print("FAILED: the exit status should be 0")
            return 1
        with open(env["NATRON_TEST_OUTPUT"]) as f:
            steps = json.load(f)
        ok = True
        for got, expected in zip(steps, EXPECTED):
            if got != expected:
                print("FAILED: [label, projectFileHasGuiData, "
                      "projectLoadedWithErrors] is %r, expected %r"
                      % (got, expected))
                ok = False
        if len(steps) != len(EXPECTED):
            print("FAILED: %d steps, expected %d" % (len(steps), len(EXPECTED)))
            ok = False
        if ok:
            print("project load info test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
