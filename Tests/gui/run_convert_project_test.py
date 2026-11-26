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
text -> binary -> text, and text -> python -> text, then the Python project
to binary with the default destination (its name with the .ntp extension).
Each file must have the requested format, and load with the same nodes, links
and parameter values (keyframes, expressions) as the original. A destination
whose extension is not the one of the format is refused; extensions are
case-insensitive; a Python project converted to Python without a destination
is saved in place; without a destination, an existing file next to the
project is not replaced. The projects saved by the Natron GUI,
Tests/data/ProjectGuiData.ntp and ProjectGuiData.py (node positions, sizes and
colours, and a Viewer), keep their GUI data in text -> binary -> text, and
keep the positions, sizes and colours of their nodes in conversions to and
from the Python format; those, which lose the Viewer, are done only with
discard_gui=True, and the Python project without a Viewer that the first one
gives converts to text and back to Python without it. A copy with a node whose
plug-in is missing (.ntp or .py) is not converted (exit status 1, no output
file). The other load errors are tested by run_project_load_info_test.py.

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

BINARY_MAGIC = b"\x01\x16serialization::archive"
# The first or second line of a Python project (NATRON_PROJECT_PYTHON_MAGIC)
PYTHON_MAGIC = b"# Natron Project"
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

# Loads NATRON_DUMP_INPUT and writes [projectFileHasGuiData(), {name:
# [position, size, colour]}] for the names NODES (see GUI_TEST_NODES in
# natronrunner.py) to NATRON_DUMP_OUTPUT (JSON).
GUI_DUMP_SCRIPT = r"""
import json
import os
import traceback

from NatronEngine import natron

NODES = %r

status = 1
try:
    app = natron.getInstance(0)
    loaded = app.loadProject(os.environ["NATRON_DUMP_INPUT"])
    if loaded is None:
        raise RuntimeError("load failed")
    nodes = {}
    for name in NODES:
        names = name.split(".")
        n = loaded.getNode(names[0])
        for child in names[1:]:
            n = n.getNode(child) if n is not None else None
        nodes[name] = None if n is None else [
            [round(v) for v in n.getPosition()],
            [round(v) for v in n.getSize()],
            [round(v, 2) for v in n.getColor()]]
    with open(os.environ["NATRON_DUMP_OUTPUT"], "w") as f:
        json.dump([loaded.projectFileHasGuiData(), nodes], f)
    status = 0
except Exception:  # anything: print it, and quit with status 1
    traceback.print_exc()
natron.quit(status)
""" % (sorted(natronrunner.GUI_TEST_NODES),)


class Runner:
    """Runs NatronRenderer with a private cache and environment."""

    def __init__(self, renderer, cmd_wrapper, tmp_dir):
        self.renderer = renderer
        self.wrapper = cmd_wrapper
        self.env = natronrunner.natron_env(
            tmp_dir, NATRON_DISK_CACHE_PATH=os.path.join(tmp_dir, "cache"))
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
        status, out = natronrunner.run(cmd, dict(self.env, **extra_env),
                                       TIMEOUT, stdin=self.stdin)
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
    """Returns "binary", "text", "python" or None, from the start of path."""
    if not os.path.isfile(path):
        return None
    with open(path, "rb") as f:
        head = f.read(256)
    if head.startswith(BINARY_MAGIC):
        return "binary"
    if head.lstrip(b"\xef\xbb\xbf \t\r\n").startswith(b"<?xml"):
        return "text"
    if PYTHON_MAGIC in [line.strip() for line in head.splitlines()[:2]]:
        return "python"
    return None


def write_variant(data, old, new, path):
    """Writes data, with its first old replaced by new, to path."""
    with open(path, "wb") as f:
        f.write(natronrunner.replace_first(data, old, new))


class ConvertProjectTest:
    """The checks; the failures are collected in self.failures."""

    def __init__(self, runner, tmp_dir):
        self.runner = runner
        self.tmp_dir = tmp_dir
        self.failures = []
        self.reference = None  # the dump of the original, see test_chains()
        self.dump_script = self.path("dump.py")
        with open(self.dump_script, "w") as f:
            f.write(DUMP_SCRIPT)
        self.gui_dump_script = self.path("gui_dump.py")
        with open(self.gui_dump_script, "w") as f:
            f.write(GUI_DUMP_SCRIPT)
        self.src = self.path("original.ntp")
        shutil.copyfile(natronrunner.TEST_PROJECT, self.src)
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

    def dump(self, path, script=None):
        """Returns the nodes and parameters of the project path (JSON).

        With script, what that dump script writes instead (see
        GUI_DUMP_SCRIPT).
        """
        out = path + ".json"
        status, _ = self.runner.run(
            ["-t", script or self.dump_script],
            {"NATRON_DUMP_INPUT": path, "NATRON_DUMP_OUTPUT": out})
        if not self.check(status == 0 and os.path.isfile(out),
                          "could not load %s" % path):
            return None
        with open(out) as f:
            return json.load(f)

    def test_chains(self):
        """text -> binary -> text and text -> python -> text -> ... binary.

        Each file has the nodes and values of the original. The last
        conversion has no destination: it is the Python project with the
        .ntp extension.
        """
        self.reference = self.dump(self.src)
        if not self.check(self.reference and
                          "Group1" in self.reference["nodes"] and
                          "Dot1" in self.reference["nodes"],
                          "the reference project has no Group1 or Dot1"):
            return
        self.convert_chain((("binary", "1.bin.ntp"), ("text", "2.text.ntp")))
        self.convert_chain((("python", "3.py"), ("text", "4.text.ntp")))
        self.convert_chain((("binary", None),), self.path("3.py"),
                           self.path("3.ntp"))

    def convert_chain(self, steps, src=None, default_dst=None):
        """Converts src (default: the original) with each (to, name) of steps.

        Each conversion converts the file of the previous one. A name None
        gives no destination: the file is then default_dst.
        """
        prev = src or self.src
        for to, name in steps:
            if name:
                dst = self.path(name)
                result = self.runner.convert(prev, dst, to)
            else:
                dst = default_dst
                result = self.runner.convert(prev, to=to)
            if not self.check_converted(result, dst, to,
                                        "%s -> %s" % (prev, to)):
                return
            self.check(self.dump(dst) == self.reference,
                       "%s does not have the nodes and values of the original"
                       % dst)
            prev = dst

    def test_extension_mismatch(self):
        """A destination without the extension of the format is refused."""
        for to, name, ext in (("text", "mismatch.py", ".ntp"),
                              ("binary", "mismatch.txt", ".ntp"),
                              ("python", "mismatch.ntp", ".py")):
            dst = self.path(name)
            self.check_refused(self.runner.convert(self.src, dst, to), dst,
                               "ValueError: %s: a %s project must have the "
                               "%s extension" % (dst, to, ext),
                               "%s to %s" % (to, name))

    def check_gui_data(self, path, what):
        """The project path must have the GUI data of the GUI test projects.

        Those that a conversion keeps: projectFileHasGuiData(), and the
        positions, sizes and colours of the nodes.
        """
        dump = self.dump(path, self.gui_dump_script)
        if dump is None:
            return
        has_gui_data, nodes = dump
        self.check(has_gui_data, "%s: %s has no GUI data" % (what, path))
        for name, expected in sorted(natronrunner.GUI_TEST_NODES.items()):
            self.check(nodes.get(name) == expected,
                       "%s: %s has [position, size, colour] %r, expected %r"
                       % (what, name, nodes.get(name), expected))

    def check_viewer(self, path, expected, what):
        """The project path must have a Viewer if expected, else none."""
        with open(path, "rb") as f:
            has_viewer = b"fr.inria.built-in.Viewer" in f.read()
        self.check(has_viewer == expected, "%s: %s %s a Viewer" % (
            what, path, "has no" if expected else "has"))

    def convert_gui_project(self, src, dst, to, what, refused=True):
        """Converts src, which has GUI data, to dst; checks dst.

        If refused (src or dst is a Python project, and src has a Viewer),
        the conversion is refused without discard_gui, and converted with it,
        with a warning. dst must have the GUI data of src, and a Viewer if
        refused is False and src has one.

        Returns:
            True if dst was converted.
        """
        if refused:
            self.check_refused(self.runner.convert(src, dst, to), dst,
                               "RuntimeError: %s has Viewers" % src, what)
            result = self.runner.convert(src, dst, to, discard_gui=True)
            what += ", discard_gui=True"
        else:
            result = self.runner.convert(src, dst, to)
        if not self.check_converted(result, dst, to, what):
            return False
        if refused:
            self.check("the Viewers of %s are not converted" % src
                       in result[1], "%s: no warning" % what)
        self.check_gui_data(dst, what)
        return True

    def test_gui_data(self):
        """The projects saved by the GUI keep their GUI data.

        All of them in .ntp files, their node positions, sizes and colours in
        conversions to and from the Python format, which lose their Viewer:
        those are done only with discard_gui=True.
        """
        ntp = self.path("gui.ntp")
        shutil.copyfile(natronrunner.GUI_TEST_PROJECT, ntp)
        binary = self.path("gui.bin.ntp")
        text = self.path("gui.text.ntp")
        if (self.convert_gui_project(ntp, binary, "binary", "GUI .ntp -> "
                                     "binary", refused=False) and
                self.convert_gui_project(binary, text, "text", "GUI binary "
                                         "-> text", refused=False)):
            self.check_viewer(text, True, "GUI binary -> text")
        # the Viewer is also found in a binary project
        dst = self.path("gui.bin.py")
        self.check_refused(self.runner.convert(binary, dst, "python"), dst,
                           "RuntimeError: %s has Viewers" % binary,
                           "GUI binary -> python")
        python = self.path("gui.py")
        if self.convert_gui_project(ntp, python, "python",
                                    "GUI .ntp -> python"):
            self.check_viewer(python, False, "GUI .ntp -> python")
            # without a Viewer, the Python project converts both ways
            back = self.path("gui.py.ntp")
            if self.convert_gui_project(python, back, "text", "GUI python "
                                        "without a Viewer -> text",
                                        refused=False):
                self.convert_gui_project(back, self.path("gui2.py"), "python",
                                         "GUI text without a Viewer -> "
                                         "python", refused=False)
        # saved as Python by the GUI, with its Viewer
        python = self.path("guiviewer.py")
        shutil.copyfile(natronrunner.GUI_TEST_PYTHON_PROJECT, python)
        dst = self.path("guiviewer.py.ntp")
        if self.convert_gui_project(python, dst, "text", "GUI python -> text"):
            self.check_viewer(dst, False, "GUI python -> text")

    def python_project(self):
        """Returns the Python project written by test_chains(), or None."""
        python = self.path("3.py")
        if not self.check(file_format(python) == "python",
                          "%s was not written by test_chains()" % python):
            return None
        with open(python, "rb") as f:
            return f.read()

    def test_extension_rules(self):
        """Extensions in upper case, in-place and existing destinations.

        Needs the Python project of test_chains().
        """
        dst = self.path("upper.NTP")
        self.check_converted(self.runner.convert(self.src, dst, "binary"),
                             dst, "binary", "binary to upper.NTP")
        data = self.python_project()
        if data is None:
            return
        # in place: the project is rebuilt, and saved again in the same file
        in_place = self.path("inplace.py")
        with open(in_place, "wb") as f:
            f.write(data)
        if self.check_converted(self.runner.convert(in_place, to="python"),
                                in_place, "python", "python in place"):
            self.check(self.dump(in_place) == self.reference,
                       "%s does not have the nodes and values of the original"
                       % in_place)
        # the file next to the project that dst=None stands for is kept
        src = self.path("exists.ntp")
        shutil.copyfile(self.src, src)
        script = self.path("exists.py")
        script_data = b"print('a script, not a project')\n"
        with open(script, "wb") as f:
            f.write(script_data)
        status, out = self.runner.convert(src, to="python")
        what = "python next to an existing script"
        self.check(status == 1, "%s: exit status %d, expected 1"
                   % (what, status))
        self.check("FileExistsError: %s exists" % script in out,
                   "%s: no FileExistsError" % what)
        with open(script, "rb") as f:
            self.check(f.read() == script_data,
                       "%s: %s was replaced" % (what, script))

    def test_python_load_error(self):
        """A Python project with a missing plug-in is not loaded.

        Needs the Python project of test_chains().
        """
        data = self.python_project()
        if data is None:
            return
        missing = self.path("missingplugin.py")
        write_variant(data, b'"fr.inria.built-in.Dot"',
                      b'"fr.inria.built-in.NoSuchPlugin"', missing)
        dst = self.path("missingplugin.py.ntp")
        self.check_refused(self.runner.convert(missing, dst, "binary"), dst,
                           "RuntimeError: %s could not be loaded" % missing,
                           "a Python project with a missing plug-in")

    def test_load_error(self):
        """A project that loads with errors (a missing plug-in) is refused."""
        missing = self.path("missingplugin.ntp")
        write_variant(self.data, natronrunner.DOT_PLUGIN_TAG,
                      natronrunner.MISSING_PLUGIN_TAG, missing)
        dst = self.path("missingplugin.bin.ntp")
        self.check_refused(self.runner.convert(missing, dst, "binary"), dst,
                           "RuntimeError: there were errors while loading %s"
                           % missing, "a project that loads with errors")


def main(argv):
    """Runs the test; returns the exit status."""
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    cmd_wrapper = natronrunner.wrapper(argv[2:])
    tmp_dir = tempfile.mkdtemp(prefix="natron-convert-project-test-")
    try:
        runner = Runner(argv[1], cmd_wrapper, tmp_dir)
        test = ConvertProjectTest(runner, tmp_dir)
        test.test_chains()
        test.test_extension_mismatch()
        test.test_gui_data()
        test.test_extension_rules()
        test.test_python_load_error()
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
