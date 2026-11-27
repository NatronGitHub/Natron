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

"""Headless test of App.saveProjectAsPython() and of loading its file.

Usage: run_project_python_test.py /path/to/NatronRenderer [--debugger]

1. "NatronRenderer -t build.py" builds a project with built-in nodes only
   (no OpenFX plug-in is needed): project settings (frame range and rate, an
   added format, a project path, an afterProjectLoad callback), a Dot, a
   Backdrop, a Group with an Input, an Output and a Dot, user parameters with
   keyframes (one linear, one with its own tangents) and an expression, a
   parametric curve, connections, and a Read of a small image if
   the openfx-io plug-ins are found (OFX_PLUGIN_PATH). (NatronRenderer keeps the
   node positions, sizes and colours that the script sets.)
   It saves it with app.saveProjectAsPython() to first.py, and writes the
   state of the project (every node, parameter and project parameter) to
   built.json. Then, after a save as text.ntp, app.saveProjectAs(savedas.py)
   without a format must write the same file (the extension gives the
   format), and give the project the Python format.
2. "NatronRenderer -t load.py" loads first.py with app.loadProject(), checks
   that the afterProjectLoad callback ran and that
   app.projectLoadedWithErrors() is False, writes the state to loaded.json and
   saves the project again to second.py, which must be first.py
   (idempotence). Loading a project with a Viewer, as the GUI saves it, works
   (NatronRenderer has no Viewers). With the openfx-misc plug-ins, a project
   with Roto shapes in a sub-layer is loaded and saved three times: the
   second and third files must be the same, and the shape keeps its
   parameters. The
   loaded project has the Python format: app.saveProject()
   writes first.py again, and app.saveProjectAs(savedasntp.ntp) without a
   format writes an .ntp file (the binary-by-default preference). It also checks that app.loadProject() refuses a PyPlug and
   a plain script.
3. "NatronRenderer -t -l onload.py first.py" loads first.py from the command
   line, as a project, and writes the state to cli.json.

The three states must be the same.

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

TIMEOUT = 120  # seconds, for each run of NatronRenderer

# The state of the project as JSON: the project parameters, then the nodes
# with their parameters (values at a few frames, to compare animations and
# expressions), recursively. Shared by the three scripts.
DUMP = r'''
import json
import os
import re

import NatronEngine
from NatronEngine import natron

SAMPLE_TIMES = (1, 2, 5, 10)
# the nodes of the multi-plane case (see BUILD)
LAYERED_NODES = ("Layered", "LayerShuffle", "AllPlanes")


def _param_state(param):
    dims = param.getNumDimensions()
    state = {"type": type(param).__name__}
    if isinstance(param, NatronEngine.ParametricParam):
        state["values"] = [[param.getValue(d, x) for x in (0.25, 0.5, 0.75)]
                           for d in range(dims)]
    elif isinstance(param, (NatronEngine.IntParam, NatronEngine.DoubleParam,
                            NatronEngine.ColorParam)):
        state["values"] = [[param.getValueAtTime(t, d) for t in SAMPLE_TIMES]
                           for d in range(dims)]
    elif hasattr(param, "getValueAtTime"):
        # choice, boolean, string: one dimension
        state["values"] = [param.getValueAtTime(t) for t in SAMPLE_TIMES]
    elif hasattr(param, "getValue"):
        state["values"] = param.getValue()
    if hasattr(param, "getExpression"):
        state["expressions"] = [str(param.getExpression(d))
                                for d in range(dims)]
    return state


def _node_state(node):
    inputs = []
    for i in range(node.getMaxInputCount()):
        inp = node.getInput(i)
        inputs.append(inp.getScriptName() if inp else None)
    params = {}
    for p in node.getParams():
        params[p.getScriptName()] = _param_state(p)
    rod = node.getRegionOfDefinition(1, 0)
    state = {
        "plugin": node.getPluginID(),
        # a Read must read its file (see AppInstance::runPythonProjectScript())
        "rod": [rod.x1, rod.y1, rod.x2, rod.y2],
        "label": node.getLabel(),
        "position": [round(v) for v in node.getPosition()],
        "size": [round(v) for v in node.getSize()],
        "color": [round(v, 3) for v in node.getColor()],
        "inputs": inputs,
        "params": params,
        "children": _nodes_state(node),
    }
    if node.getScriptName() == "Params":
        # a user parameter without a value (see PARAMS_PYPLUG); a separator
        # is not saved, in a .ntp file either
        apply_button = node.getParam("apply")
        state["shown"] = [apply_button.getIsVisible(),
                          apply_button.getIsEnabled(0)]
    if node.getScriptName() in LAYERED_NODES:
        # the planes that the node can produce, with the ones a user added
        state["layers"] = sorted(layer.getLayerName()
                                 for layer in node.getAvailableLayers(-1))
    return state


def _nodes_state(group):
    return {n.getScriptName(): _node_state(n) for n in group.getChildren()}


def dump(a, path):
    project = {}
    for name in ("frameRange", "frameRate", "outputFormat", "comments",
                 "afterProjectLoad", "lockRange"):
        project[name] = _param_state(a.getProjectParam(name))
    # without [Project], the folder of the file, which a new project doesn't have
    paths = re.sub("<Name>Project</Name><Value>[^<]*</Value>", "",
                   a.getProjectParam("projectPaths").getValue())
    with open(path, "w") as f:
        json.dump({"project": project, "projectPaths": paths,
                   "nodes": _nodes_state(a)}, f, indent=1, sort_keys=True)
'''

BUILD = DUMP + r'''
import traceback

status = 1
try:
    a = app
    a.getProjectParam("frameRange").setValue(3, 0)
    a.getProjectParam("frameRange").setValue(42, 1)
    a.getProjectParam("lockRange").setValue(True)
    a.getProjectParam("frameRate").setValue(30)
    a.addFormat("TestFormat 320x200 2")
    a.getProjectParam("outputFormat").set("TestFormat")
    a.getProjectParam("comments").setValue("a \"quoted\"\ncomment")
    a.getProjectParam("afterProjectLoad").setValue("testAfterProjectLoad")
    paths = a.getProjectParam("projectPaths")
    paths.setValue(paths.getValue()
                   + "<Name>Footage</Name><Value>/footage</Value>")

    dot = a.createNode("fr.inria.built-in.Dot")
    dot.setScriptName("Source")
    dot.setPosition(10, 20)
    backdrop = a.createNode("fr.inria.built-in.BackDrop")
    backdrop.setScriptName("Notes")
    backdrop.setLabel("Some notes")
    backdrop.setPosition(-100, -50)
    backdrop.setSize(400, 300)
    backdrop.setColor(0.5, 0.25, 0.125)

    grp = a.createNode("fr.inria.built-in.Group")
    grp.setScriptName("Grp")
    grp.setPosition(200, 100)
    page = grp.createPageParam("controls", "Controls")
    amount = grp.createDoubleParam("amount", "Amount")
    page.addParam(amount)
    amount.setValueAtTime(1.5, 1)
    amount.setValueAtTime(4.25, 10)
    # a keyframe that is not smooth, and one with its own tangents (the
    # sampled values differ otherwise)
    amount.setInterpolationAtTime(1, NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeLinear)
    if not amount.setDerivativesAtTime(10, 0.5, 0.25):
        raise RuntimeError("setDerivativesAtTime() failed")
    # a parametric curve with a control point that has its own tangents
    curves = grp.createParametricParam("curves", "Curves", 1)
    page.addParam(curves)
    curves.deleteAllControlPoints(0)
    curves.addControlPoint(0, 0, 0)
    curves.addControlPoint(0, 0.5, 0.75, 3, -1, NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeFree)
    curves.addControlPoint(0, 1, 1)
    twice = grp.createDoubleParam("twice", "Twice")
    page.addParam(twice)
    twice.setExpression("thisNode.amount.get() * 2", False)
    grp.refreshUserParamsGUI()
    inp = a.createNode("fr.inria.built-in.Input", -1, grp)
    inp.setScriptName("In")
    inner = a.createNode("fr.inria.built-in.Dot", -1, grp)
    inner.setScriptName("InnerDot")
    inner.connectInput(0, inp)
    out = None
    for n in grp.getChildren():
        if n.getPluginID() == "fr.inria.built-in.Output":
            out = n
    if out is None:
        out = a.createNode("fr.inria.built-in.Output", -1, grp)
    out.connectInput(0, inner)
    grp.connectInput(0, dot)

    # a Read, with openfx-io (as in CI)
    read = None
    if "fr.inria.openfx.ReadOIIO" in natron.getPluginIDs():
        read = a.createReader(os.environ["NATRON_TEST_IMAGE"])
        read.setScriptName("Image")
        if read.getRegionOfDefinition(1, 0).x2 != 4:
            raise RuntimeError("the Read didn't read its image")
    else:
        print("no ReadOIIO plug-in: the Read is not tested")

    # An instance of an installed PyPlug: its parameters, and the ones of a
    # node inside it
    params = a.createNode("fr.natron.test.ProjectPythonParams")
    params.setScriptName("Params")
    params.getParam("amount").setValue(3)
    params.getNode("Inner").getParam("disableNode").setValue(True)
    params.getParam("apply").setVisible(False)
    params.getParam("apply").setEnabled(False, 0)

    # Multiple planes: a node that writes to a plane that a user added, a
    # Shuffle that takes the channels of that plane (openfx-misc), and a
    # Write of all the planes (openfx-io), with a plane of its own
    ids = natron.getPluginIDs()
    layered = None
    if ("net.sf.openfx.ConstantPlugin" in ids
            and "net.sf.openfx.ShufflePlugin" in ids):
        constant = a.createNode("net.sf.openfx.ConstantPlugin")
        constant.setScriptName("Layered")
        if not constant.addUserPlane("MyLayer", ["R", "G", "B", "A"]):
            raise RuntimeError("addUserPlane() failed")
        constant.getParam("channels").set("MyLayer")
        constant.getParam("color").setValue(0.25, 0)
        layered = a.createNode("net.sf.openfx.ShufflePlugin")
        layered.setScriptName("LayerShuffle")
        layered.connectInput(0, constant)
        for channel in "RGBA":
            layered.getParam("output" + channel).set("A.MyLayer." + channel)
    else:
        print("no Constant or Shuffle plug-in: the user plane is not tested")
    if "fr.inria.openfx.WriteOIIO" in ids:
        write = a.createWriter(os.path.join(
            os.path.dirname(os.environ["NATRON_TEST_BUILT"]), "planes.exr"))
        write.setScriptName("AllPlanes")
        if not write.addUserPlane("WriteLayer", ["X", "Y"]):
            raise RuntimeError("addUserPlane() failed")
        write.getParam("processAllPlanes").setValue(True)
        if layered is not None:
            write.connectInput(0, layered)
        elif read is not None:
            write.connectInput(0, read)
    else:
        print("no WriteOIIO plug-in: the Write of all planes is not tested")

    dump(a, os.environ["NATRON_TEST_BUILT"])
    if not a.saveProjectAsPython(os.environ["NATRON_TEST_FIRST"]):
        raise RuntimeError("saveProjectAsPython() failed")
    # it doesn't change the project
    if a.getProjectParam("projectName").getValue() == "first.py":
        raise RuntimeError("saveProjectAsPython() renamed the project")
    # Save As "x.py" of a text .ntp project, without a format: the extension
    # gives the Python format, which the project gets
    formats = NatronEngine.Natron.ProjectFileFormatEnum
    if not a.saveProjectAs(os.environ["NATRON_TEST_TEXTNTP"],
                           formats.eProjectFileFormatText):
        raise RuntimeError("saveProjectAs(eProjectFileFormatText) failed")
    if not a.saveProjectAs(os.environ["NATRON_TEST_SAVEDAS"]):
        raise RuntimeError("saveProjectAs(.py) failed")
    if a.getProjectFormat() != formats.eProjectFileFormatPython:
        raise RuntimeError("getProjectFormat() is %r" % a.getProjectFormat())
    status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
'''

LOAD = DUMP + r'''
import traceback

called = []


def testAfterProjectLoad(app):
    called.append(app.getProjectParam("projectName").getValue())


status = 1
try:
    a = natron.getInstance(0)
    first = os.environ["NATRON_TEST_FIRST"]
    for not_a_project in (os.environ["NATRON_TEST_PYPLUG"],
                          os.environ["NATRON_TEST_SCRIPT"]):
        if a.loadProject(not_a_project) is not None:
            raise RuntimeError("%s was loaded as a project" % not_a_project)
    with open(first, "rb") as f:
        first_contents = f.read()
    with_viewer = a.loadProject(os.environ["NATRON_TEST_WITHVIEWER"])
    if with_viewer is None or with_viewer.getNode("Dot1") is None:
        raise RuntimeError("the project with a Viewer was not loaded")
    # Roto shapes, as a PyPlug or a project creates them (see WITH_ROTO)
    if "net.sf.openfx.RotoPlugin" in natron.getPluginIDs():
        saved = []
        path = os.environ["NATRON_TEST_WITHROTO"]
        # the fixture is not written by Natron: from the second save on
        for i in range(3):
            project = a.loadProject(path)
            if project is None:
                raise RuntimeError("%s was not loaded" % path)
            shape = project.getNode("Shapes").getRotoContext().getItemByName("Disc")
            linear = NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeLinear
            if (shape.getParam("opacity").getValue() != 0.5 or not shape.getLocked()
                    or shape.getKeyframeInterpolation(1) != linear):
                raise RuntimeError("the Roto shape lost its parameters")
            path = os.environ["NATRON_TEST_ROTO%d" % i]
            if not project.saveProjectAsPython(path):
                raise RuntimeError("saveProjectAsPython() failed")
            with open(path, "rb") as f:
                saved.append(f.read())
        if saved[1] != saved[2]:
            raise RuntimeError("the Roto project was not saved as the same file")
    else:
        print("no Roto plug-in: the Roto shapes are not tested")
    loaded = a.loadProject(first)
    if loaded is None:
        raise RuntimeError("app.loadProject() failed")
    if called != ["first.py"]:
        raise RuntimeError("afterProjectLoad: %r" % called)
    if loaded.projectLoadedWithErrors():
        raise RuntimeError("projectLoadedWithErrors() is True")
    # [Project] is the folder of the file
    paths = loaded.getProjectParam("projectPaths").getValue()
    # Natron's paths have forward slashes, also on Windows
    folder = os.path.dirname(first).replace("\\", "/")
    if "<Name>Project</Name><Value>%s</Value>" % folder not in paths:
        raise RuntimeError("[Project] is not %s: %s" % (folder, paths))
    # the copies expand [Project], as saveProject() does
    for save, name in ((loaded.saveProjectCopy, "copy.ntp"),
                       (loaded.saveTempProject, "temp.ntp")):
        if not save("[Project]/" + name):
            raise RuntimeError("%s() failed" % save.__name__)
        if not os.path.isfile(os.path.join(os.path.dirname(first), name)):
            raise RuntimeError("%s() did not expand [Project]" % save.__name__)
    dump(loaded, os.environ["NATRON_TEST_LOADED"])
    if not loaded.saveProjectAsPython(os.environ["NATRON_TEST_SECOND"]):
        raise RuntimeError("saveProjectAsPython() failed")
    # "Save" writes the script again
    python = NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatPython
    if loaded.getProjectFormat() != python:
        raise RuntimeError("getProjectFormat() is %r" % loaded.getProjectFormat())
    if not loaded.saveProject(""):
        raise RuntimeError("saveProject() failed")
    with open(first, "rb") as f:
        if f.read() != first_contents:
            raise RuntimeError("saveProject() did not write first.py again")
    # Save As "x.ntp" of a Python project, without a format: the "Save
    # projects as binary by default" preference
    if not loaded.saveProjectAs(os.environ["NATRON_TEST_SAVEDASNTP"]):
        raise RuntimeError("saveProjectAs(.ntp) failed")
    with open(os.environ["NATRON_TEST_SAVEDASNTP"], "rb") as f:
        if b"# Natron Project" in f.read(64):
            raise RuntimeError("saveProjectAs(.ntp) wrote a Python project")
    if loaded.getProjectFormat() == python:
        raise RuntimeError("getProjectFormat() is %r" % loaded.getProjectFormat())
    status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
'''

ONLOAD = DUMP + r'''
import traceback

status = 1
try:
    if app.getProjectParam("projectName").getValue() != "first.py":
        raise RuntimeError("first.py was not loaded as a project")
    dump(app, os.environ["NATRON_TEST_CLI"])
    status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
'''

# Not projects: app.loadProject() must refuse them
PYPLUG = """# -*- coding: utf-8 -*-
# Natron PyPlug

def getPluginID():
    return "fr.natron.test.ProjectPythonPyPlug"


def createInstance(app, group):
    pass
"""
SCRIPT = "app.createNode('fr.inria.built-in.Dot')\n"

# An installed PyPlug (found through NATRON_PLUGIN_PATH), with a button and a
# separator, which have no value, and a node inside it whose parameters an
# instance changes (see BUILD)
PARAMS_PYPLUG = """# -*- coding: utf-8 -*-


def getPluginID():
    return "fr.natron.test.ProjectPythonParams"


def getLabel():
    return "ProjectPythonParams"


def getVersion():
    return 1


def createInstance(app, group):
    page = group.createPageParam("controls", "Controls")
    amount = group.createIntParam("amount", "Amount")
    amount.setDefaultValue(7, 0)
    amount.restoreDefaultValue(0)
    page.addParam(amount)
    page.addParam(group.createButtonParam("apply", "Apply"))
    page.addParam(group.createSeparatorParam("line", "Line"))
    group.refreshUserParamsGUI()
    inner = app.createNode("fr.inria.built-in.Group", -1, group)
    inner.setScriptName("Inner")
    app.createNode("fr.inria.built-in.Output", -1, group)
"""

# A project with a Roto shape in a sub-layer, locked, with its own opacity and
# a linear keyframe,
# as the exporter writes it (Project::exportToPython())
WITH_ROTO = """# -*- coding: utf-8 -*-
# Natron Project

import NatronEngine


def createProject(app):
    group = app
    lastNode = app.createNode("fr.inria.built-in.Roto", 1, group)
    lastNode.setScriptName("Shapes")
    roto = lastNode.getRotoContext()
    Layer1_layer = roto.getBaseLayer()
    Sub_layer = roto.createLayer()
    Sub_layer.setScriptName("Sub")
    Layer1_layer.addItem(Sub_layer)
    bezier = roto.createEllipse(100, 100, 50, True, 1)
    bezier.setScriptName("Disc")
    bezier.getOpacityParam().setValue(0.5)
    bezier.setKeyframeInterpolation(1, NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeLinear)
    Sub_layer.addItem(bezier)
    bezier.setLocked(True)


if __name__ == "__main__":
    createProject(app)
"""

# A project with a Viewer, as the GUI saves it (Project::exportToPython()):
# NatronRenderer, which has no Viewers, loads it without the Viewer.
WITH_VIEWER = """# -*- coding: utf-8 -*-
# Natron Project

import NatronEngine


def createProject(app):
    group = app
    lastNode = app.createNode("fr.inria.built-in.Dot", 1, group)
    lastNode.setScriptName("Dot1")
    groupDot1 = lastNode
    del lastNode
    # Viewer "Viewer1"
    groupViewer1 = app.createNode("fr.inria.built-in.Viewer", 1, group)
    if groupViewer1 is not None:
        groupViewer1.setScriptName("Viewer1")
        groupViewer1.setPosition(10, 20)

    if groupViewer1 is not None:
        groupViewer1.connectInput(0, groupDot1)


if __name__ == "__main__":
    createProject(app)
"""


def run_renderer(cmd, env):
    """Runs NatronRenderer, prints its output.

    Returns:
      True if it exited with status 0.
    """
    status, output = natronrunner.run(cmd, env, TIMEOUT)
    sys.stdout.write(output)
    if status is None:
        print("FAILED: timeout")
        return False
    print("exit status %d" % status)
    if status != 0:
        print("FAILED: the exit status should be 0")
        return False
    return True


def _write(path, contents):
    """Writes contents (str) to path, in UTF-8.

    Returns:
      path.
    """
    with open(path, "w", encoding="utf-8") as f:
        f.write(contents)
    return path


def _compare(label, expected, got):
    """Prints the differences between two states (dicts).

    Returns:
      True if they are the same.
    """
    if expected == got:
        return True
    print("FAILED: %s: the state differs from the built project" % label)
    _print_differences("", expected, got)
    return False


def _print_differences(path, expected, got):
    """Prints the differences between two JSON values, recursively.

    Args:
      path: the location of the values in the state, e.g. "/nodes/Dot1".
      expected: the value of the built project.
      got: the value of the loaded project.
    """
    if isinstance(expected, dict) and isinstance(got, dict):
        for key in sorted(set(expected) | set(got)):
            if key not in got:
                print("  %s/%s: missing" % (path, key))
            elif key not in expected:
                print("  %s/%s: unexpected" % (path, key))
            else:
                _print_differences("%s/%s" % (path, key), expected[key],
                                   got[key])
    elif expected != got:
        print("  %s: %r, expected %r" % (path, got, expected))


def write_test_files(tmp_dir, env):
    """Writes the input files and the scripts, and sets their paths in env.

    Args:
      tmp_dir: the directory to write them to.
      env: the environment of NatronRenderer, updated with NATRON_TEST_*.

    Returns:
      A dict of the paths, by name ("first", "build"...).
    """
    files = {}
    for name in ("built", "loaded", "cli"):
        files[name] = os.path.join(tmp_dir, name + ".json")
    for name in ("first", "second", "savedas"):
        files[name] = os.path.join(tmp_dir, name + ".py")
    files["savedasntp"] = os.path.join(tmp_dir, "savedasntp.ntp")
    files["textntp"] = os.path.join(tmp_dir, "text.ntp")
    files["pyplug"] = _write(os.path.join(tmp_dir, "pyplug.py"), PYPLUG)
    files["script"] = _write(os.path.join(tmp_dir, "script.py"), SCRIPT)
    pyplugs_dir = os.path.join(tmp_dir, "pyplugs")
    os.mkdir(pyplugs_dir)
    _write(os.path.join(pyplugs_dir, "ProjectPythonParams.py"), PARAMS_PYPLUG)
    env["NATRON_PLUGIN_PATH"] = os.pathsep.join(
        p for p in (pyplugs_dir, env.get("NATRON_PLUGIN_PATH")) if p)
    # a 4x2 image for the Read (binary PPM)
    files["image"] = os.path.join(tmp_dir, "image.ppm")
    with open(files["image"], "wb") as f:
        f.write(b"P6\n4 2\n255\n" + bytes(range(24)))
    files["withroto"] = _write(os.path.join(tmp_dir, "withroto.py"), WITH_ROTO)
    files["roto0"] = os.path.join(tmp_dir, "roto0.py")
    files["roto1"] = os.path.join(tmp_dir, "roto1.py")
    files["roto2"] = os.path.join(tmp_dir, "roto2.py")
    files["withviewer"] = _write(os.path.join(tmp_dir, "withviewer.py"),
                                 WITH_VIEWER)
    for name, path in files.items():
        env["NATRON_TEST_" + name.upper()] = path
    for name, script in (("build", BUILD), ("load", LOAD),
                         ("onload", ONLOAD)):
        files[name] = _write(os.path.join(tmp_dir, name + ".py"), script)
    return files


def _check_results(files):
    """Compares the three states, and the files saved from them.

    Args:
      files: the paths, see write_test_files().

    Returns:
      True if everything matches.
    """
    states = {}
    for name in ("built", "loaded", "cli"):
        with open(files[name], encoding="utf-8") as f:
            states[name] = json.load(f)
    ok = _compare("app.loadProject()", states["built"], states["loaded"])
    ok = _compare("command line", states["built"], states["cli"]) and ok
    with open(files["first"], "rb") as f:
        first = f.read()
    with open(files["second"], "rb") as f:
        if f.read() != first:
            print("FAILED: the project loaded from first.py was not saved"
                  " as the same file")
            ok = False
    with open(files["savedas"], "rb") as f:
        if f.read() != first:
            print("FAILED: saveProjectAs(savedas.py) and"
                  " saveProjectAsPython() wrote different files")
            ok = False
    if not ok:
        print("first.py:\n" + first.decode())
    return ok


def main(argv):
    """Runs the test, see the module docstring.

    Returns:
      The exit status.
    """
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    renderer = natronrunner.wrapper(argv[2:]) + [argv[1], "--no-settings"]
    tmp_dir = os.path.realpath(
        tempfile.mkdtemp(prefix="natron-project-python-test-"))
    try:
        env = natronrunner.natron_env(
            tmp_dir, NATRON_DISK_CACHE_PATH=os.path.join(tmp_dir, "cache"))
        files = write_test_files(tmp_dir, env)
        if not (run_renderer(renderer + ["-t", files["build"]], env)
                and run_renderer(renderer + ["-t", files["load"]], env)
                and run_renderer(renderer + ["-t", "-l", files["onload"],
                                              files["first"]], env)):
            return 1
        if not _check_results(files):
            return 1
        print("project python test passed")
        return 0
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
