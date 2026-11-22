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

NatronRenderer -t loads, one after the other, Tests/data/ProjectFileFormat-2.5.ntp
(saved in background mode, no error) and variants of it, written by this
test:

- "gui": flagged as saved by the GUI (Background_project=0). It is
  synthetic: the flag is flipped, but the file has no ProjectGui part. The
  background app doesn't read that part anyway.
- "missingplugin", "missingingroup": a node whose plug-in does not exist, at
  the top level and in a Group.
- "missinginput": a connection to an input that the node doesn't have.
- "majorversion": a node saved with another major version of its plug-in,
  a warning where nothing is lost.
- "callback": in a Group, a node whose plug-in does not exist, flagged as saved
  by the GUI, with an afterProjectLoad callback that reads both getters.

It reads both getters for a new project, after each load (also a clean load,
"aftererrors", right after one that lost data), in the afterProjectLoad
callback and after app.resetProject(), and checks that each load prints its
own error log messages (none for the clean loads). Only the text (XML) format is tested: both formats are
read by the same code once the archive is open.

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
GUI_TAG = b"<Background_project>0</Background_project>"
DOT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Dot</Plugin_id>"
# the only Input node of the project is in Group1
INPUT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Input</Plugin_id>"
MISSING_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.NoSuchPlugin</Plugin_id>"
# the input of the Output node of Group1
SOURCE_INPUT_TAG = b"<first>Source</first>"
MISSING_INPUT_TAG = b"<first>NoSuchInput</first>"
MAJOR_VERSION_TAG = b"<Plugin_major_version>1</Plugin_major_version>"
OTHER_MAJOR_VERSION_TAG = b"<Plugin_major_version>7</Plugin_major_version>"
KNOBS_COUNT_TAG = b"<ProjectKnobsCount>4</ProjectKnobsCount>"
FORMATS_TAG = b"\t<AdditionalFormats"
CALLBACK = "loadInfoCallback"
CALLBACK_KNOB = (b"""\t<item>
\t\t<Name>afterProjectLoad</Name>
\t\t<Type>String</Type>
\t\t<Dimension>1</Dimension>
\t\t<Secret>0</Secret>
\t\t<MasterIsAlias>0</MasterIsAlias>
\t\t<item>
\t\t\t<Enabled>1</Enabled>
\t\t\t<HasAnimation>0</HasAnimation>
\t\t\t<Value>%s</Value>
\t\t\t<Default></Default>
\t\t\t<HasMaster>0</HasMaster>
\t\t\t<Expression></Expression>
\t\t\t<ExprHasRet>0</ExprHasRet>
\t\t</item>
\t\t<StringsAnimation>
\t\t\t<count>0</count>
\t\t\t<item_version>0</item_version>
\t\t</StringsAnimation>
\t\t<UserKnob>0</UserKnob>
\t</item>
""" % CALLBACK.encode())

# Printed by the script before each load
LOAD_MARKER = "NATRON_TEST_LOAD"

# The projects that the script loads, in this order (NATRON_TEST_<LABEL>).
LOADS = ("background", "gui", "missingplugin", "aftererrors",
         "missingingroup", "missinginput", "majorversion", "callback")

# Run by NatronRenderer -t: writes [label, hasGuiData, loadedWithErrors] for
# each step to NATRON_TEST_OUTPUT (JSON).
SCRIPT = r"""
import json
import os
import traceback

from NatronEngine import natron

LOADS = %r
LOAD_MARKER = %r
callback_states = []


def state(label, a):
    return [label, a.projectFileHasGuiData(), a.projectLoadedWithErrors()]


def %s(app):
    callback_states.append(state("in callback", app))


status = 1
try:
    a = natron.getInstance(0)
    steps = [state("new", a)]
    for label in LOADS:
        # the output of each load starts with this line (see _split_output())
        print("%%s %%s" %% (LOAD_MARKER, label), flush=True)
        loaded = a.loadProject(os.environ["NATRON_TEST_" + label.upper()])
        if loaded is None:
            steps.append([label, "load failed"])
        else:
            steps.append(state(label, loaded))
            a = loaded
    steps.extend(callback_states)
    a.resetProject()
    steps.append(state("reset", a))
    with open(os.environ["NATRON_TEST_OUTPUT"], "w") as f:
        json.dump(steps, f)
    status = 0
except BaseException:
    traceback.print_exc()
natron.quit(status)
""" % (LOADS, LOAD_MARKER, CALLBACK)

EXPECTED = [
    ["new", False, False],
    ["background", False, False],
    ["gui", True, False],
    ["missingplugin", False, True],
    ["aftererrors", False, False],
    ["missingingroup", False, True],
    ["missinginput", False, True],
    ["majorversion", False, False],
    ["callback", True, True],
    ["in callback", True, True],
    ["reset", False, False],
]

# The messages that the output of each load must have, which show that its
# case was really exercised. NatronRenderer prints the error log after a load
# that opens it. The log is never cleared, so a later load prints these again:
# each message must occur more often than in the last error log printed
# before. The loads that are not listed must print no error log line.
EXPECTED_MESSAGES = {
    "missingplugin": ["fr.inria.built-in.NoSuchPlugin version 1.0 was found "
                      "in the script but does not exist in the loaded "
                      "plug-ins.\n"],
    "missingingroup": ["does not exist in the loaded plug-ins. It was in the "
                       "group Group1 (Group <&> \u00e9)."],
    "missinginput": ["Could not find input named NoSuchInput"],
    "majorversion": ["was found in the script but was loaded with version"],
    "callback": ["does not exist in the loaded plug-ins. It was in the "
                 "group Group1 (Group <&> \u00e9)."],
}
# A line of the error log printed by NatronRenderer
ERROR_LOG_LINE = "] Project: "


def _replace(data, old, new, after=None):
    """Replaces the first occurrence of old, or the first one after after.

    Args:
      data: the project file contents (bytes).
      old: the bytes to replace.
      new: the replacement.
      after: if given, only an occurrence of old after the first occurrence
        of after is replaced.

    Returns:
      The new contents.

    Raises:
      ValueError: old (or after) is not in data.
    """
    start = 0
    if after is not None:
        start = data.find(after)
        if start == -1:
            raise ValueError("%r not found in the test project" % after)
    index = data.find(old, start)
    if index == -1:
        raise ValueError("%r not found in the test project" % old)
    return data[:index] + new + data[index + len(old):]


def _variants(data):
    """Returns the test projects, by label (see LOADS).

    Raises:
      ValueError: the test project doesn't have the expected contents.
    """
    missing_in_group = _replace(data, INPUT_PLUGIN_TAG, MISSING_PLUGIN_TAG)
    callback = _replace(_replace(missing_in_group, BACKGROUND_TAG, GUI_TAG),
                        KNOBS_COUNT_TAG,
                        b"<ProjectKnobsCount>5</ProjectKnobsCount>")
    callback = _replace(callback, FORMATS_TAG, CALLBACK_KNOB + FORMATS_TAG)
    return {
        "background": data,
        "gui": _replace(data, BACKGROUND_TAG, GUI_TAG),
        "missingplugin": _replace(data, DOT_PLUGIN_TAG, MISSING_PLUGIN_TAG),
        "aftererrors": data,
        "missingingroup": missing_in_group,
        "missinginput": _replace(data, SOURCE_INPUT_TAG, MISSING_INPUT_TAG),
        "majorversion": _replace(data, MAJOR_VERSION_TAG,
                                 OTHER_MAJOR_VERSION_TAG,
                                 after=DOT_PLUGIN_TAG),
        "callback": callback,
    }


def _write_test_files(tmp_dir, env):
    """Writes the projects and the script, and sets their paths in env.

    Args:
      tmp_dir: the directory to write them to.
      env: the environment of NatronRenderer, updated.

    Returns:
      The path of the script.

    Raises:
      OSError: a file can't be read or written.
      ValueError: the test project doesn't have the expected contents.
    """
    with open(PROJECT, "rb") as f:
        data = f.read()
    for label, contents in _variants(data).items():
        path = os.path.join(tmp_dir, label + ".ntp")
        with open(path, "wb") as f:
            f.write(contents)
        env["NATRON_TEST_" + label.upper()] = path
    env["NATRON_TEST_OUTPUT"] = os.path.join(tmp_dir, "out.json")
    script = os.path.join(tmp_dir, "load_info.py")
    with open(script, "w") as f:
        f.write(SCRIPT)
    return script


def _run_renderer(cmd, env):
    """Runs NatronRenderer, prints its output.

    Returns:
      Its output, or None if it failed or timed out.
    """
    print("Running: " + " ".join(cmd))
    sys.stdout.flush()
    try:
        proc = subprocess.run(cmd, env=env, stdin=subprocess.DEVNULL,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=300)
    except subprocess.TimeoutExpired:
        print("FAILED: timeout")
        return None
    # natron-lldb.sh runs it in a terminal, which ends the lines with \r\n
    output = proc.stdout.decode(errors="replace").replace("\r\n", "\n")
    sys.stdout.write(output)
    print("exit status %d" % proc.returncode)
    if proc.returncode != 0:
        print("FAILED: the exit status should be 0")
        return None
    return output


def _split_output(output):
    """Splits the output of NatronRenderer by load.

    Returns:
      The labels of LOADS in order, and a dict of the output of each load
      (from its LOAD_MARKER line to the next one).
    """
    labels = []
    outputs = {}
    label = None
    for line in output.splitlines(True):
        if line.startswith(LOAD_MARKER + " "):
            label = line[len(LOAD_MARKER) + 1:].strip()
            labels.append(label)
            outputs[label] = ""
        elif label is not None:
            outputs[label] += line
    return labels, outputs


def _check_messages(output):
    """Checks that each load logged its own messages, and the clean ones none.

    Returns:
      True if they match EXPECTED_MESSAGES.
    """
    labels, outputs = _split_output(output)
    if labels != list(LOADS):
        print("FAILED: the loads in the output are %r, expected %r"
              % (labels, list(LOADS)))
        return False
    ok = True
    last_log = ""  # the output of the last load that printed the error log
    for label in labels:
        messages = EXPECTED_MESSAGES.get(label)
        if messages is None:
            if ERROR_LOG_LINE in outputs[label]:
                print("FAILED: %s: the load printed the error log" % label)
                ok = False
            continue
        for message in messages:
            if outputs[label].count(message) <= last_log.count(message):
                print("FAILED: %s: the load did not log %r" % (label, message))
                ok = False
        last_log = outputs[label]
    return ok


def _check(steps, output):
    """Compares the steps and the output with the expected ones.

    Returns:
      True if they match.
    """
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
    return _check_messages(output) and ok


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
        env = dict(os.environ)
        env["NATRON_DISK_CACHE_PATH"] = os.path.join(tmp_dir, "cache")
        # Debug builds: no trapped floating point exception (#1537)
        env["NATRON_NO_FP_TRAPS"] = "1"
        script = _write_test_files(tmp_dir, env)
        output = _run_renderer(
            wrapper + [argv[1], "--no-settings", "-t", script], env)
        if output is None:
            return 1
        with open(env["NATRON_TEST_OUTPUT"]) as f:
            steps = json.load(f)
        if not _check(steps, output):
            return 1
        print("project load info test passed")
        return 0
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
