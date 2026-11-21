#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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

"""Headless test of tools/natron-convert-project.py, run by NatronRenderer.

Usage: run_convert_project_test.py /path/to/NatronRenderer [--debugger]

Tests/data/ProjectFileFormat-2.5.ntp (a text project saved by Natron 2.5.0, with a group, user
parameters, animation and an expression) is converted text -> binary -> text -> binary -> text.
Each file must have the requested format, the two text files written by the converter must be
identical except for the save date, and every project must load with the same nodes, links and
parameter values (keyframes, expressions) as the original. A copy with GUI data is not converted
unless NATRON_CONVERT_DISCARD_GUI=1, a copy with a node whose plug-in is missing is not converted,
and a bad NATRON_CONVERT_TO or a missing input fails.

Exit status: 0 on success, 1 on failure.
--debugger runs NatronRenderer under natron-lldb.sh (see run_gui_smoke_test.py), or set
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
SRCDIR = os.path.dirname(os.path.dirname(HERE))
CONVERTER = os.path.join(SRCDIR, "tools", "natron-convert-project.py")
PROJECT = os.path.join(SRCDIR, "Tests", "data", "ProjectFileFormat-2.5.ntp")
BINARY_MAGIC = b"\x01\x16serialization::archive"

# Loads NATRON_DUMP_INPUT and writes a description of its nodes and parameters to NATRON_DUMP_OUTPUT (JSON).
DUMP_SCRIPT = r"""
import json, os, sys
import NatronEngine
from NatronEngine import natron

def dumpParam(p):
    d = {"type": type(p).__name__, "dims": []}
    for dim in range(p.getNumDimensions()):
        e = {}
        if hasattr(p, "getValue"):
            try:
                e["value"] = p.getValue(dim)
            except TypeError:
                e["value"] = p.getValue()
        if hasattr(p, "getNumKeys"):
            keys = []
            for i in range(p.getNumKeys(dim)):
                ok, t = p.getKeyTime(i, dim)
                keys.append([t, p.getValueAtTime(t, dim)])
            e["keys"] = keys
        if hasattr(p, "getExpression"):
            e["expression"] = list(p.getExpression(dim))
        d["dims"].append(e)
    return d

def dumpNodes(group, out, prefix=""):
    for n in group.getChildren():
        name = prefix + n.getScriptName()
        out[name] = {
            "plugin": n.getPluginID(),
            "label": n.getLabel(),
            "inputs": [(n.getInput(i).getScriptName() if n.getInput(i) else None) for i in range(n.getMaxInputCount())],
            "params": dict((p.getScriptName(), dumpParam(p)) for p in n.getParams()),
        }
        if n.getPluginID() == "fr.inria.built-in.Group":
            dumpNodes(n, out, name + ".")

status = 1
try:
    app = natron.getInstance(0)
    loaded = app.loadProject(os.environ["NATRON_DUMP_INPUT"])
    assert loaded is not None, "load failed"
    nodes = {}
    dumpNodes(loaded, nodes)
    project = {}
    for name in ("frameRange", "outputFormat", "frameRate", "colorSpace8u"):
        p = loaded.getProjectParam(name)
        if p is not None:
            project[name] = dumpParam(p)
    with open(os.environ["NATRON_DUMP_OUTPUT"], "w") as f:
        json.dump({"nodes": nodes, "project": project}, f, indent=1, sort_keys=True, default=str)
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
"""


class Runner(object):
    def __init__(self, renderer, wrapper, tmpDir):
        self.renderer = renderer
        self.wrapper = wrapper
        self.env = dict(os.environ)
        # no image cache in the user's directory
        self.env["NATRON_DISK_CACHE_PATH"] = os.path.join(tmpDir, "cache")
        # Debug builds: no trapped floating point exception (#1537), as with Natron --test
        self.env["NATRON_NO_FP_TRAPS"] = "1"
        for k in ("NATRON_CONVERT_TO", "NATRON_CONVERT_INPUT", "NATRON_CONVERT_OUTPUT", "NATRON_CONVERT_DISCARD_GUI"):
            self.env.pop(k, None)

    def run(self, script, extraEnv):
        env = dict(self.env)
        env.update(extraEnv)
        cmd = self.wrapper + [self.renderer, "--no-settings", "-t", script]
        print("Running: %s %s" % (" ".join("%s=%s" % kv for kv in sorted(extraEnv.items())), " ".join(cmd)))
        sys.stdout.flush()
        try:
            proc = subprocess.run(cmd, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, timeout=300)
        except subprocess.TimeoutExpired:
            print("FAILED: timeout")
            return -1, ""
        out = proc.stdout.decode(errors="replace")
        # the plug-in loading messages are not interesting
        sys.stdout.write("".join(l for l in out.splitlines(True) if not l.startswith("OpenFX: loading")))
        print("exit status %d" % proc.returncode)
        return proc.returncode, out

    def convert(self, to, src, dst, extraEnv=None):
        env = {"NATRON_CONVERT_TO": to, "NATRON_CONVERT_INPUT": src, "NATRON_CONVERT_OUTPUT": dst}
        env.update(extraEnv or {})
        return self.run(CONVERTER, env)


def fileFormat(path):
    with open(path, "rb") as f:
        head = f.read(64)
    if head.startswith(BINARY_MAGIC):
        return "binary"
    if head.lstrip(b"\xef\xbb\xbf \t\r\n").startswith(b"<?xml"):
        return "text"
    return None


def textWithoutSaveDate(path):
    # The lastSaveDate parameter of the project changes at each save: drop its value
    lines = open(path, "rb").read().decode("utf-8").splitlines()
    out = []
    skipValue = False
    for line in lines:
        if "<Name>lastSaveDate</Name>" in line:
            skipValue = True
        elif skipValue and "<Value>" in line:
            line = "<Value>(save date)</Value>"
            skipValue = False
        out.append(line)
    return out


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    tmpDir = tempfile.mkdtemp(prefix="natron-convert-project-test-")
    failures = []

    def check(cond, msg):
        if not cond:
            print("FAILED: " + msg)
            failures.append(msg)
        return cond

    try:
        r = Runner(argv[1], wrapper, tmpDir)
        dumpScript = os.path.join(tmpDir, "dump.py")
        with open(dumpScript, "w") as f:
            f.write(DUMP_SCRIPT)

        def dump(path):
            out = path + ".json"
            status, _ = r.run(dumpScript, {"NATRON_DUMP_INPUT": path, "NATRON_DUMP_OUTPUT": out})
            if not check(status == 0 and os.path.isfile(out), "could not load %s" % path):
                return None
            with open(out) as f:
                return json.load(f)

        src = os.path.join(tmpDir, "original.ntp")
        shutil.copyfile(PROJECT, src)
        reference = dump(src)
        check(reference and "Group1" in reference["nodes"] and "Dot1" in reference["nodes"],
              "the reference project has no Group1 or Dot1")

        # text -> binary -> text -> binary -> text
        chain = [("binary", "1.bin.ntp"), ("text", "2.text.ntp"), ("binary", "3.bin.ntp"), ("text", "4.text.ntp")]
        prev = src
        for to, name in chain:
            dst = os.path.join(tmpDir, name)
            status, out = r.convert(to, prev, dst)
            if not check(status == 0 and os.path.isfile(dst), "converting %s to %s failed" % (prev, to)):
                return 1
            check(fileFormat(dst) == to, "%s is not a %s project" % (dst, to))
            check(dump(dst) == reference, "%s does not have the nodes and values of the original" % dst)
            prev = dst
        check(textWithoutSaveDate(os.path.join(tmpDir, "2.text.ntp")) == textWithoutSaveDate(os.path.join(tmpDir, "4.text.ntp")),
              "2.text.ntp and 4.text.ntp differ (other than the save date)")

        # in place
        inPlace = os.path.join(tmpDir, "inplace.ntp")
        shutil.copyfile(src, inPlace)
        status, _ = r.run(CONVERTER, {"NATRON_CONVERT_TO": "binary", "NATRON_CONVERT_INPUT": inPlace})
        check(status == 0 and fileFormat(inPlace) == "binary", "in-place conversion failed")

        # A project with GUI data (only the flag: NatronRenderer does not read the GUI part) is refused
        withGui = os.path.join(tmpDir, "withgui.ntp")
        data = open(src, "rb").read()
        check(b"<Background_project>1</Background_project>" in data, "unexpected test project")
        with open(withGui, "wb") as f:
            f.write(data.replace(b"<Background_project>1</Background_project>", b"<Background_project>0</Background_project>", 1))
        dst = os.path.join(tmpDir, "withgui.bin.ntp")
        status, out = r.convert("binary", withGui, dst)
        check(status == 1 and not os.path.exists(dst) and "has GUI data" in out,
              "a project with GUI data should not be converted")
        status, out = r.convert("binary", withGui, dst, {"NATRON_CONVERT_DISCARD_GUI": "1"})
        check(status == 0 and fileFormat(dst) == "binary", "NATRON_CONVERT_DISCARD_GUI=1 should convert it")

        # A project with a node whose plug-in is missing loads, without that node: it is not converted
        missingPlugin = os.path.join(tmpDir, "missingplugin.ntp")
        check(b"<Plugin_id>fr.inria.built-in.Dot</Plugin_id>" in data, "unexpected test project")
        with open(missingPlugin, "wb") as f:
            f.write(data.replace(b"<Plugin_id>fr.inria.built-in.Dot</Plugin_id>",
                                 b"<Plugin_id>fr.inria.built-in.NoSuchPluginForTheTest</Plugin_id>", 1))
        dst = os.path.join(tmpDir, "missingplugin.bin.ntp")
        status, out = r.convert("binary", missingPlugin, dst)
        check(status == 1 and not os.path.exists(dst) and "errors while loading" in out,
              "a project that loads with errors should not be converted")

        # bad arguments
        dst = os.path.join(tmpDir, "bad.ntp")
        status, out = r.convert("xml", src, dst)
        check(status == 1 and not os.path.exists(dst), "NATRON_CONVERT_TO=xml should fail")
        status, out = r.convert("text", os.path.join(tmpDir, "missing.ntp"), dst)
        check(status == 1 and not os.path.exists(dst), "a missing input should fail")

        if failures:
            print("%d failure(s)" % len(failures))
            return 1
        print("convert project test passed")
        return 0
    finally:
        shutil.rmtree(tmpDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
