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

"""Headless test of the natron_convert_project module (tools/).

Usage: run_convert_project_test.py /path/to/NatronRenderer [--debugger]

The module must be importable by NatronRenderer: the build copies it to the
Plugins directory of the build tree, which is on the Python path of the
build-tree NatronRenderer, as in an installed Natron.

Each conversion runs NatronRenderer -t -c "import natron_convert_project as c;
c.convert_and_quit(...)", with commands on the standard input that must not
run: convert_and_quit() must end the session, without starting the prompt.
Tests/data/ProjectFileFormat-2.5.ntp (a text project saved by Natron 2.5.0,
with a group, user parameters, animation and an expression) is converted
text -> binary -> text. Each file must have the requested format, and load
with the same nodes, links and parameter values (keyframes, expressions) as
the original. A copy with GUI data is converted only with discard_gui=True,
and a copy with a node whose plug-in is missing is not converted (exit status
1, no output file). The other load errors are tested by
run_project_load_info_test.py.

Exit status: 0 on success, 1 on failure, 2 on a usage error.
--debugger runs NatronRenderer under natron-lldb.sh, or set
NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import json
import os
import shutil
import sys
import tempfile

from natronrunner import (BACKGROUND_TAG, DOT_PLUGIN_TAG, GUI_TAG,
                          MISSING_PLUGIN_TAG, TEST_PROJECT, natron_env,
                          replace_first, run, wrapper)

BINARY_MAGIC = b"\x01\x16serialization::archive"
# Given to the -t prompt, which must not start after convert_and_quit()
PROMPT_STDIN = 'import sys; sys.stdout.write("PROMPT STARTED\\n")\n'
PROMPT_MARKER = "PROMPT STARTED"
TIMEOUT = 60  # seconds, for each run of NatronRenderer (about 2 s here)

# Loads NATRON_DUMP_INPUT and writes a description of its nodes and
# parameters to NATRON_DUMP_OUTPUT (JSON).
DUMP_SCRIPT = r"""
import json
import os
import traceback

from NatronEngine import natron

def dump_param(p):
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
                _, t = p.getKeyTime(i, dim)
                keys.append([t, p.getValueAtTime(t, dim)])
            e["keys"] = keys
        if hasattr(p, "getExpression"):
            e["expression"] = list(p.getExpression(dim))
        d["dims"].append(e)
    return d

def dump_nodes(group, out, prefix=""):
    for n in group.getChildren():
        name = prefix + n.getScriptName()
        inputs = [n.getInput(i) for i in range(n.getMaxInputCount())]
        out[name] = {
            "plugin": n.getPluginID(),
            "label": n.getLabel(),
            "inputs": [i.getScriptName() if i else None for i in inputs],
            "params": {p.getScriptName(): dump_param(p)
                       for p in n.getParams()},
        }
        if n.getPluginID() == "fr.inria.built-in.Group":
            dump_nodes(n, out, name + ".")

status = 1
try:
    app = natron.getInstance(0)
    loaded = app.loadProject(os.environ["NATRON_DUMP_INPUT"])
    if loaded is None:
        raise RuntimeError("load failed")
    nodes = {}
    dump_nodes(loaded, nodes)
    project = {}
    for name in ("frameRange", "outputFormat", "frameRate", "colorSpace8u"):
        p = loaded.getProjectParam(name)
        if p is not None:
            project[name] = dump_param(p)
    with open(os.environ["NATRON_DUMP_OUTPUT"], "w") as f:
        json.dump({"nodes": nodes, "project": project}, f, indent=1,
                  sort_keys=True, default=str)
    status = 0
except Exception:  # anything: print it, and quit with status 1
    traceback.print_exc()
natron.quit(status)
"""


class Runner:
    """Runs NatronRenderer with a private cache and environment."""

    def __init__(self, renderer, cmd_wrapper, tmp_dir):
        self.renderer = renderer
        self.wrapper = cmd_wrapper
        self.env = natron_env(tmp_dir)
        self.stdin = os.path.join(tmp_dir, "prompt_stdin.py")
        with open(self.stdin, "w") as f:
            f.write(PROMPT_STDIN)

    def run(self, args, extra_env=None):
        """Runs NatronRenderer --no-settings args; returns (status, output).

        The standard input is PROMPT_STDIN. The status is -1 after a timeout.
        """
        extra_env = extra_env or {}
        if extra_env:
            print("With %s:" % " ".join("%s=%s" % kv
                                        for kv in sorted(extra_env.items())))
        cmd = self.wrapper + [self.renderer, "--no-settings"] + args
        status, out = run(cmd, dict(self.env, **extra_env), TIMEOUT,
                          stdin=self.stdin)
        # the plug-in loading messages are not interesting
        sys.stdout.write("".join(line for line in out.splitlines(True)
                                 if not line.startswith("OpenFX: loading")))
        if status is None:
            print("FAILED: timeout")
            return -1, out
        print("exit status %d" % status)
        return status, out

    def convert(self, *args, **kwargs):
        """Runs natron_convert_project.convert_and_quit(*args, **kwargs)."""
        call = ", ".join([repr(a) for a in args] +
                         ["%s=%r" % kv for kv in sorted(kwargs.items())])
        return self.run(["-t", "-c", "import natron_convert_project as c; "
                         "c.convert_and_quit(%s)" % call])


def file_format(path):
    """Returns "binary", "text" or None, from the first bytes of path."""
    with open(path, "rb") as f:
        head = f.read(64)
    if head.startswith(BINARY_MAGIC):
        return "binary"
    if head.lstrip(b"\xef\xbb\xbf \t\r\n").startswith(b"<?xml"):
        return "text"
    return None


def write_variant(data, old, new, path):
    """Writes data, with its first old replaced by new, to path."""
    with open(path, "wb") as f:
        f.write(replace_first(data, old, new))


class ConvertProjectTest:
    """The checks; the failures are collected in self.failures."""

    def __init__(self, runner, tmp_dir):
        self.runner = runner
        self.tmp_dir = tmp_dir
        self.failures = []
        self.dump_script = self.path("dump.py")
        with open(self.dump_script, "w") as f:
            f.write(DUMP_SCRIPT)
        self.src = self.path("original.ntp")
        shutil.copyfile(TEST_PROJECT, self.src)
        with open(self.src, "rb") as f:
            self.data = f.read()

    def path(self, name):
        """Returns the path of name in the temporary directory."""
        return os.path.join(self.tmp_dir, name)

    def check(self, cond, msg):
        """Records the failure msg unless cond; returns cond."""
        if not cond:
            print("FAILED: " + msg)
            self.failures.append(msg)
        return cond

    def check_no_prompt(self, result, what):
        """The -t prompt must not have run the commands of its input."""
        return self.check(PROMPT_MARKER not in result[1],
                          "%s: the -t prompt started" % what)

    def check_converted(self, result, dst, to, what):
        """The conversion must succeed and write dst in the format to."""
        status, _ = result
        return (self.check(status == 0, "%s: exit status %d, expected 0"
                           % (what, status)) and
                self.check_no_prompt(result, what) and
                self.check(file_format(dst) == to,
                           "%s: %s is not a %s project" % (what, dst, to)))

    def check_refused(self, result, dst, error, what):
        """The conversion must fail with the exception line error."""
        status, out = result
        self.check(status == 1, "%s: exit status %d, expected 1"
                   % (what, status))
        self.check_no_prompt(result, what)
        self.check(error in out, "%s: %r is not in the output" % (what, error))
        self.check(not os.path.exists(dst), "%s: %s was written" % (what, dst))

    def dump(self, path):
        """Returns the nodes and parameters of the project path (JSON)."""
        out = path + ".json"
        status, _ = self.runner.run(
            ["-t", self.dump_script],
            {"NATRON_DUMP_INPUT": path, "NATRON_DUMP_OUTPUT": out})
        if not self.check(status == 0 and os.path.isfile(out),
                          "could not load %s" % path):
            return None
        with open(out) as f:
            return json.load(f)

    def test_chain(self):
        """text -> binary -> text, same nodes and values."""
        reference = self.dump(self.src)
        if not self.check(reference and "Group1" in reference["nodes"] and
                          "Dot1" in reference["nodes"],
                          "the reference project has no Group1 or Dot1"):
            return
        prev = self.src
        for to, name in (("binary", "1.bin.ntp"), ("text", "2.text.ntp")):
            dst = self.path(name)
            if not self.check_converted(self.runner.convert(prev, dst, to),
                                        dst, to, "%s -> %s" % (prev, to)):
                return
            self.check(self.dump(dst) == reference,
                       "%s does not have the nodes and values of the original"
                       % dst)
            prev = dst

    def test_gui_data(self):
        """A project with GUI data is converted only with discard_gui."""
        # Only the flag: NatronRenderer does not read the GUI part
        with_gui = self.path("withgui.ntp")
        write_variant(self.data, BACKGROUND_TAG, GUI_TAG, with_gui)
        dst = self.path("withgui.bin.ntp")
        self.check_refused(self.runner.convert(with_gui, dst, "binary"), dst,
                           "RuntimeError: %s has GUI data" % with_gui,
                           "a project with GUI data")
        result = self.runner.convert(with_gui, dst, "binary", discard_gui=True)
        if self.check_converted(result, dst, "binary", "discard_gui=True"):
            self.check("the GUI data of %s are not converted" % with_gui
                       in result[1], "discard_gui=True: no warning")

    def test_load_error(self):
        """A project that loads with errors (a missing plug-in) is refused."""
        missing = self.path("missingplugin.ntp")
        write_variant(self.data, DOT_PLUGIN_TAG, MISSING_PLUGIN_TAG, missing)
        dst = self.path("missingplugin.bin.ntp")
        self.check_refused(self.runner.convert(missing, dst, "binary"), dst,
                           "RuntimeError: there were errors while loading %s"
                           % missing, "a project that loads with errors")


def main(argv):
    """Runs the test; returns the exit status."""
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    cmd_wrapper = wrapper(argv[2:])
    tmp_dir = tempfile.mkdtemp(prefix="natron-convert-project-test-")
    try:
        runner = Runner(argv[1], cmd_wrapper, tmp_dir)
        test = ConvertProjectTest(runner, tmp_dir)
        test.test_chain()
        test.test_gui_data()
        test.test_load_error()
        if test.failures:
            print("%d failure(s)" % len(test.failures))
            return 1
        print("convert project test passed")
        return 0
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
