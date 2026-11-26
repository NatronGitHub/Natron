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
Tests/data/ProjectFileFormat-2.5.ntp (saved in background mode, no error)
and variants of it, written by this test:

- "gui": Tests/data/ProjectGuiData.ntp, saved by the GUI.
- "nogui": flagged as saved by the GUI (Background_project=0), but without
  the ProjectGui part that follows: the background app, which keeps the GUI
  data, warns, and the load has errors (saving would lose the GUI data).
- "missingplugin", "missingingroup": a node whose plug-in does not exist, at
  the top level and in a Group.
- "missinginput": a connection to an input that the node doesn't have.
- "majorversion": a node saved with another major version of its plug-in,
  a warning where nothing is lost.
- "missingpyplug": a Group saved as a PyPlug whose .py does not exist: it is
  loaded as a Group, and saving would drop the PyPlug.
- "pyplug": a Group saved as a PyPlug that is found in the PyPlug search path
  (NATRON_PLUGIN_PATH), not at its saved path: nothing is lost.
- "callback": in a Group, a node whose plug-in does not exist, flagged as saved
  by the GUI (as "nogui"), with an afterProjectLoad callback that reads both
  getters.

It reads both getters for a new project, after each load (also a clean load,
"aftererrors", right after one that lost data), in the afterProjectLoad
callback and after app.resetProject(). The "majorversion" case has no
effect on the getters, so the test also checks that its warning is printed, as
for "nogui".
Only the text (XML) format is tested: both formats are read by the same code
once the archive is open.

Exit status: 0 on success, 1 on failure, 2 on a usage error.
--debugger runs NatronRenderer under natron-lldb.sh, or set
NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import json
import os
import shutil
import sys
import tempfile

import natronrunner

TIMEOUT = 300  # seconds

# the only Input node of the project is in Group1
INPUT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Input</Plugin_id>"
# the input of the Output node of Group1
SOURCE_INPUT_TAG = b"<first>Source</first>"
MISSING_INPUT_TAG = b"<first>NoSuchInput</first>"
MAJOR_VERSION_TAG = b"<Plugin_major_version>1</Plugin_major_version>"
OTHER_MAJOR_VERSION_TAG = b"<Plugin_major_version>7</Plugin_major_version>"
# Group1, the first node of the project
GROUP_PYTHON_MODULE_TAG = b"<PythonModule></PythonModule>"
MISSING_PYTHON_MODULE_TAG = (b"<PythonModule>/nonexistent/"
                             b"NatronTestNoSuchPyPlug.py</PythonModule>")
PYPLUG_MODULE = "NatronTestLoadInfoPyPlug"
PYTHON_MODULE_TAG = (b"<PythonModule>/nonexistent/%s.py</PythonModule>"
                     % PYPLUG_MODULE.encode())
# A minimal PyPlug, which the PyPlug scan at startup recognizes
PYPLUG = """def getPluginID():
    return "fr.natron.test.LoadInfoPyPlug"


def getLabel():
    return "LoadInfoPyPlug"


def getVersion():
    return 1


def getGrouping():
    return "Other"


def createInstance(app, group):
    pass
"""
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

# The projects that the script loads, in this order (NATRON_TEST_<LABEL>).
LOADS = ("background", "gui", "nogui", "missingplugin", "aftererrors",
         "missingingroup", "missinginput", "majorversion", "missingpyplug",
         "pyplug", "callback")

# Run by NatronRenderer -t: writes [label, hasGuiData, loadedWithErrors] for
# each step to NATRON_TEST_OUTPUT (JSON).
SCRIPT = r"""
import json
import os
import traceback

from NatronEngine import natron

LOADS = %r
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
""" % (LOADS, CALLBACK)

EXPECTED = [
    ["new", False, False],
    ["background", False, False],
    ["gui", True, False],
    ["nogui", True, True],
    ["missingplugin", False, True],
    ["aftererrors", False, False],
    ["missingingroup", False, True],
    ["missinginput", False, True],
    ["majorversion", False, False],
    ["missingpyplug", False, True],
    ["pyplug", False, False],
    ["callback", True, True],
    ["in callback", True, True],
    ["reset", False, False],
]

# Printed by the "majorversion" load, which doesn't change the getters
MAJOR_VERSION_MESSAGE = "was found in the script but was loaded with version"
# Printed by the "nogui" load (Project::loadProjectInternal())
NO_GUI_MESSAGE = "has no GUI data (node positions"


def _variants(data, gui_data):
    """Returns the test projects, by label (see LOADS).

    Raises:
      ValueError: the test project doesn't have the expected contents.
    """
    # short names for the table below
    replace = natronrunner.replace_first
    background, gui = natronrunner.BACKGROUND_TAG, natronrunner.GUI_TAG
    dot, missing = natronrunner.DOT_PLUGIN_TAG, natronrunner.MISSING_PLUGIN_TAG
    missing_in_group = replace(data, INPUT_PLUGIN_TAG, missing)
    callback = replace(replace(missing_in_group, background, gui),
                       KNOBS_COUNT_TAG,
                       b"<ProjectKnobsCount>5</ProjectKnobsCount>")
    callback = replace(callback, FORMATS_TAG, CALLBACK_KNOB + FORMATS_TAG)
    return {
        "background": data,
        "gui": gui_data,
        "nogui": replace(data, background, gui),
        "missingplugin": replace(data, dot, missing),
        "aftererrors": data,
        "missingingroup": missing_in_group,
        "missinginput": replace(data, SOURCE_INPUT_TAG, MISSING_INPUT_TAG),
        "majorversion": replace(data, MAJOR_VERSION_TAG,
                                OTHER_MAJOR_VERSION_TAG,
                                after=dot),
        "missingpyplug": replace(data, GROUP_PYTHON_MODULE_TAG,
                                 MISSING_PYTHON_MODULE_TAG),
        "pyplug": replace(data, GROUP_PYTHON_MODULE_TAG, PYTHON_MODULE_TAG),
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
    with open(natronrunner.TEST_PROJECT, "rb") as f:
        data = f.read()
    with open(natronrunner.GUI_TEST_PROJECT, "rb") as f:
        gui_data = f.read()
    for label, contents in _variants(data, gui_data).items():
        path = os.path.join(tmp_dir, label + ".ntp")
        with open(path, "wb") as f:
            f.write(contents)
        env["NATRON_TEST_" + label.upper()] = path
    env["NATRON_TEST_OUTPUT"] = os.path.join(tmp_dir, "out.json")
    pyplugs_dir = os.path.join(tmp_dir, "pyplugs")
    os.mkdir(pyplugs_dir)
    with open(os.path.join(pyplugs_dir, PYPLUG_MODULE + ".py"), "w") as f:
        f.write(PYPLUG)
    env["NATRON_PLUGIN_PATH"] = pyplugs_dir
    script = os.path.join(tmp_dir, "load_info.py")
    with open(script, "w") as f:
        f.write(SCRIPT)
    return script


def _run_renderer(cmd, env):
    """Runs NatronRenderer, prints its output.

    Returns:
      Its output, or None if it failed or timed out.
    """
    status, output = natronrunner.run(cmd, env, TIMEOUT)
    sys.stdout.write(output)
    if status is None:
        print("FAILED: timeout")
        return None
    print("exit status %d" % status)
    if status != 0:
        print("FAILED: the exit status should be 0")
        return None
    return output


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
    for label, message in (("majorversion", MAJOR_VERSION_MESSAGE),
                           ("nogui", NO_GUI_MESSAGE)):
        if message not in output:
            print("FAILED: %s: no %r in the output" % (label, message))
            ok = False
    return ok


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    cmd_wrapper = natronrunner.wrapper(argv[2:])
    tmp_dir = tempfile.mkdtemp(prefix="natron-project-load-info-test-")
    try:
        env = natronrunner.natron_env(
            tmp_dir, NATRON_DISK_CACHE_PATH=os.path.join(tmp_dir, "cache"))
        script = _write_test_files(tmp_dir, env)
        output = _run_renderer(
            cmd_wrapper + [argv[1], "--no-settings", "-t", script], env)
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
