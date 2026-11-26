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

"""Headless test: NatronRenderer keeps the GUI data of the projects it saves.

Usage: run_project_gui_data_test.py /path/to/NatronRenderer [--debugger]

Tests/data/ProjectGuiData.ntp and ProjectGuiData.py were saved by the Natron
GUI: two Dots, a Backdrop, a Group with a Dot inside, with their positions,
sizes and colours, and a Viewer. "NatronRenderer -t" loads each of them, and
saves it as text, binary and Python; it loads each saved file, and the binary
one is saved as text again. Every project must have GUI data
(App.projectFileHasGuiData()), no load error, and the positions, sizes and
colours of the original. The text files saved from the .ntp project must have
its GUI data (the ProjectGui part: the GUI data of the nodes, the window
layout, the viewer settings, the opened panels) and its Viewer. Last, the .ntp
project is edited (a node renamed, one deleted, one moved, one created with
only a position) and saved: the GUI data and the Viewer's input follow.

Exit status: 0 on success, 1 on failure, 2 on a usage error.
--debugger runs NatronRenderer under natron-lldb.sh, or set
NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import json
import os
import shutil
import sys
import tempfile
import xml.etree.ElementTree as ElementTree

import natronrunner

TIMEOUT = 300  # seconds

# The nodes of the test projects, by fully qualified name: [position, size,
# colour]
EXPECTED_NODES = {
    "Dot1": [[100, 200], [16, 16], [0.8, 0.2, 0.1]],
    "Dot2": [[100, 320], [16, 16], [0.7, 0.7, 0.7]],
    "Backdrop1": [[40, 150], [300, 260], [0.2, 0.4, 0.6]],
    "Group1": [[420, 200], [104, 30], [0.5, 0.3, 0.7]],
    "Group1.Dot3": [[10, 20], [16, 16], [0.1, 0.9, 0.3]],
}
VIEWER_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Viewer</Plugin_id>"
# in edited.ntp, only the Viewer has Renamed (Dot2) as input
VIEWER_INPUT_TAG = b"<second>Renamed</second>"
OLD_VIEWER_INPUT_TAG = b"<second>Dot2</second>"

# Run by NatronRenderer -t: writes, for each project it loads, [label,
# hasGuiData, loadedWithErrors, nodes] to NATRON_TEST_OUTPUT (JSON).
SCRIPT = r"""
import json
import os
import traceback

import NatronEngine
from NatronEngine import natron

NODES = %r
FMT = NatronEngine.Natron.ProjectFileFormatEnum
# the saves of each project: (label suffix, format, extension)
SAVES = (("text", FMT.eProjectFileFormatText, ".ntp"),
         ("binary", FMT.eProjectFileFormatBinary, ".ntp"),
         ("python", FMT.eProjectFileFormatPython, ".py"))


def node(a, name):
    names = name.split(".")
    n = a.getNode(names[0])
    for child in names[1:]:
        n = n.getNode(child) if n is not None else None
    return n


def state(label, a):
    nodes = {}
    for name in NODES:
        n = node(a, name)
        nodes[name] = None if n is None else [
            [round(v) for v in n.getPosition()],
            [round(v) for v in n.getSize()],
            [round(v, 2) for v in n.getColor()]]
    return [label, a.projectFileHasGuiData(), a.projectLoadedWithErrors(),
            nodes]


def load(a, label, path, steps):
    loaded = a.loadProject(path)
    if loaded is None:
        raise RuntimeError("%%s: %%s could not be loaded" %% (label, path))
    steps.append(state(label, loaded))
    return loaded


status = 1
try:
    out_dir = os.environ["NATRON_TEST_DIR"]
    a = natron.getInstance(0)
    steps = []
    for source in ("ntp", "py"):
        loaded = load(a, source, os.environ["NATRON_TEST_" + source.upper()],
                      steps)
        saved = []
        for suffix, file_format, ext in SAVES:
            path = os.path.join(out_dir, "%%s-%%s%%s" %% (source, suffix, ext))
            if not loaded.saveProjectAs(path, file_format):
                raise RuntimeError("%%s could not be saved" %% path)
            saved.append((source + "-" + suffix, path))
        for label, path in saved:
            reloaded = load(loaded, label, path, steps)
            if label.endswith("-binary"):
                path = os.path.join(out_dir, label + "-text.ntp")
                if not reloaded.saveProjectAs(path, FMT.eProjectFileFormatText):
                    raise RuntimeError("%%s could not be saved" %% path)
                load(reloaded, label + "-text", path, steps)
            loaded = reloaded
        a = loaded
    # a script that edits the GUI-saved project, then saves it
    edited = load(a, "edit", os.environ["NATRON_TEST_NTP"], [])
    node(edited, "Dot2").setScriptName("Renamed")
    node(edited, "Backdrop1").destroy()
    node(edited, "Dot1").setPosition(5, 6)
    new_dot = edited.createNode("fr.inria.built-in.Dot")
    new_dot.setScriptName("NewDot")
    new_dot.setPosition(7, 8)
    path = os.path.join(out_dir, "edited.ntp")
    if not edited.saveProjectAs(path, FMT.eProjectFileFormatText):
        raise RuntimeError("%%s could not be saved" %% path)
    load(edited, "edited", path, steps)
    with open(os.environ["NATRON_TEST_OUTPUT"], "w") as f:
        json.dump(steps, f)
    status = 0
except BaseException:
    traceback.print_exc()
natron.quit(status)
""" % (sorted(EXPECTED_NODES) + ["Renamed", "NewDot"],)

# The projects that the script loads, in this order
LABELS = ("ntp", "ntp-text", "ntp-binary", "ntp-binary-text", "ntp-python",
          "py", "py-text", "py-binary", "py-binary-text", "py-python", "edited")
# The nodes of edited.ntp: Dot2 renamed, Backdrop1 deleted, Dot1 moved, and a
# new Dot with only a position, which gets the default colour
EDITED_NODES = dict(EXPECTED_NODES, Dot1=[[5, 6], [16, 16], [0.8, 0.2, 0.1]],
                    Dot2=None, Backdrop1=None,
                    Renamed=EXPECTED_NODES["Dot2"],
                    NewDot=[[7, 8], [0, 0], [0.7, 0.7, 0.7]])
# The text files saved from the .ntp project, which have all its GUI data
NTP_TEXT_FILES = ("ntp-text.ntp", "ntp-binary-text.ntp")


def _elements(element):
    """Returns the tag, text and children of element, without the attributes.

    The attributes (class_id, object_id...) depend on the order in which Boost
    first writes each class.
    """
    return (element.tag, (element.text or "").strip(),
            [_elements(child) for child in element])


def _nodes_gui(element, nodes):
    """Adds the GUI data of the nodes in element (NodesGui) to nodes, by name.

    The GUI finds the nodes by their name: the order of the entries, and whether
    those of the nodes of a Group are in the entry of the Group, don't matter.
    """
    for item in element.findall("item"):
        fields = [_elements(child) for child in item if child.tag != "item"]
        nodes[item.findtext("Name")] = fields
        _nodes_gui(item, nodes)


def _gui_data(path):
    """Returns the GUI data of the text project path, or None if it has none.

    Returns:
        A dictionary: the GUI data of the nodes ("NodesGui", by name), and the
        other parts of the ProjectGui element (see _elements()).
    """
    root = ElementTree.parse(path).getroot()
    if root.findtext("Background_project") != "0":
        return None
    project_gui = root.find("ProjectGui")
    if project_gui is None:
        return None
    ret = {}
    for child in project_gui:
        if child.tag == "NodesGui":
            nodes = {}
            _nodes_gui(child, nodes)
            ret[child.tag] = nodes
        else:
            ret[child.tag] = _elements(child)
    return ret


def _check_steps(steps):
    """Checks the state of each loaded project.

    Returns:
        True if they are the expected ones.
    """
    ok = True
    labels = [step[0] for step in steps]
    if labels != list(LABELS):
        print("FAILED: loaded %r, expected %r" % (labels, list(LABELS)))
        ok = False
    for label, has_gui_data, loaded_with_errors, nodes in steps:
        if not has_gui_data:
            print("FAILED: %s: projectFileHasGuiData() is False" % label)
            ok = False
        if loaded_with_errors:
            print("FAILED: %s: projectLoadedWithErrors() is True" % label)
            ok = False
        expected_nodes = EDITED_NODES if label == "edited" else dict(
            EXPECTED_NODES, Renamed=None, NewDot=None)
        for name, expected in sorted(expected_nodes.items()):
            if nodes.get(name) != expected:
                print("FAILED: %s: %s has [position, size, colour] %r, "
                      "expected %r" % (label, name, nodes.get(name), expected))
                ok = False
    return ok


def _check_files(tmp_dir):
    """Checks the GUI data and the Viewer of the text files that were saved.

    Returns:
        True if they are the expected ones.
    """
    ok = True
    original = _gui_data(natronrunner.GUI_TEST_PROJECT)
    for name in NTP_TEXT_FILES:
        path = os.path.join(tmp_dir, name)
        saved = _gui_data(path)
        if saved != original:
            parts = sorted(set(saved or {}) | set(original))
            print("FAILED: %s: the GUI data differ from those of %s: %r"
                  % (name, natronrunner.GUI_TEST_PROJECT,
                     [p for p in parts
                      if (saved or {}).get(p) != original.get(p)]))
            ok = False
        with open(path, "rb") as f:
            if VIEWER_PLUGIN_TAG not in f.read():
                print("FAILED: %s: no Viewer" % name)
                ok = False
    # a Python project has no window layout and no viewer settings
    saved = _gui_data(os.path.join(tmp_dir, "py-text.ntp"))
    if saved is None or sorted(saved["NodesGui"]) != sorted(
            n for n in original["NodesGui"] if n != "Viewer1"):
        print("FAILED: py-text.ntp: the GUI data of the nodes are %r"
              % (saved and sorted(saved["NodesGui"])))
        ok = False
    # the GUI data and the Viewer follow the edits
    path = os.path.join(tmp_dir, "edited.ntp")
    saved = _gui_data(path)
    expected = sorted(set(original["NodesGui"]) - {"Dot2", "Backdrop1"}
                      | {"Renamed", "NewDot"})
    if saved is None or sorted(saved["NodesGui"]) != expected:
        print("FAILED: edited.ntp: the GUI data of the nodes are %r, "
              "expected %r" % (saved and sorted(saved["NodesGui"]), expected))
        ok = False
    with open(path, "rb") as f:
        data = f.read()
    if (VIEWER_PLUGIN_TAG not in data or VIEWER_INPUT_TAG not in data
            or OLD_VIEWER_INPUT_TAG in data):
        print("FAILED: edited.ntp: no Viewer connected to Renamed")
        ok = False
    return ok


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    cmd_wrapper = natronrunner.wrapper(argv[2:])
    tmp_dir = tempfile.mkdtemp(prefix="natron-project-gui-data-test-")
    try:
        env = natronrunner.natron_env(
            tmp_dir, NATRON_TEST_DIR=tmp_dir,
            NATRON_TEST_NTP=natronrunner.GUI_TEST_PROJECT,
            NATRON_TEST_PY=natronrunner.GUI_TEST_PYTHON_PROJECT,
            NATRON_TEST_OUTPUT=os.path.join(tmp_dir, "out.json"))
        script = os.path.join(tmp_dir, "gui_data.py")
        with open(script, "w") as f:
            f.write(SCRIPT)
        status, output = natronrunner.run(
            cmd_wrapper + [argv[1], "--no-settings", "-t", script], env,
            TIMEOUT)
        sys.stdout.write(output)
        if status != 0:
            print("FAILED: the exit status is %r, not 0" % status)
            return 1
        with open(env["NATRON_TEST_OUTPUT"]) as f:
            steps = json.load(f)
        ok = _check_steps(steps)
        ok = _check_files(tmp_dir) and ok
        if not ok:
            return 1
        print("project GUI data test passed")
        return 0
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
