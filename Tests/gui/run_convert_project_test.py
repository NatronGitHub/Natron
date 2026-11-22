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
text -> binary -> text -> binary -> text. Each file must have the requested
format, the two text files must be identical except for the save date, and
every project must load with the same nodes, links and parameter values
(keyframes, expressions) as the original. A copy with GUI data is converted
only with discard_gui=True, a copy with a node whose plug-in is missing is
not converted (also when that node is in a group), and bad arguments raise
the documented exceptions, with the exit status 1 and no output file.
Importing the module does nothing else.

Exit status: 0 on success, 1 on failure, 2 on a usage error.
--debugger runs NatronRenderer under natron-lldb.sh (see
run_gui_smoke_test.py), or set NATRON_TEST_WRAPPER.
"""

import glob
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
BINARY_MAGIC = b"\x01\x16serialization::archive"
# Given to the -t prompt, which must not start after convert_and_quit()
PROMPT_STDIN = 'import sys; sys.stdout.write("PROMPT STARTED\\n")\n'
PROMPT_MARKER = "PROMPT STARTED"
TIMEOUT = 120  # seconds, for each run of NatronRenderer (about 2 s here)
BACKGROUND_TAG = b"<Background_project>1</Background_project>"
DOT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Dot</Plugin_id>"
# the only Input node of the project is in Group1
INPUT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Input</Plugin_id>"
MISSING_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.NoSuchPlugin</Plugin_id>"

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

    def __init__(self, renderer, wrapper, tmp_dir):
        self.renderer = renderer
        self.wrapper = wrapper
        self.env = dict(os.environ)
        # no image cache in the user's directory
        self.env["NATRON_DISK_CACHE_PATH"] = os.path.join(tmp_dir, "cache")
        # Debug builds: no trapped floating point exception (#1537)
        self.env["NATRON_NO_FP_TRAPS"] = "1"
        self.stdin = os.path.join(tmp_dir, "prompt_stdin.py")
        with open(self.stdin, "w") as f:
            f.write(PROMPT_STDIN)
        # natron-lldb.sh reads the standard input of the program from there
        self.env["NATRON_LLDB_STDIN"] = self.stdin

    def run(self, args, extra_env=None):
        """Runs NatronRenderer --no-settings args; returns (status, output).

        The standard input is PROMPT_STDIN.
        """
        extra_env = extra_env or {}
        env = dict(self.env)
        env.update(extra_env)
        cmd = self.wrapper + [self.renderer, "--no-settings"] + args
        print("Running: %s %s" % (
            " ".join("%s=%s" % kv for kv in sorted(extra_env.items())),
            " ".join(shlex.quote(a) for a in cmd)))
        sys.stdout.flush()
        with open(self.stdin, "rb") as stdin:
            proc = subprocess.Popen(cmd, env=env, stdin=stdin,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            try:
                out, _ = proc.communicate(timeout=TIMEOUT)
            except subprocess.TimeoutExpired:
                # SIGTERM first: natron-lldb.sh then kills the program too
                proc.terminate()
                try:
                    proc.communicate(timeout=20)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.communicate()
                print("FAILED: timeout")
                return -1, ""
        out = out.decode(errors="replace")
        # the plug-in loading messages are not interesting
        sys.stdout.write("".join(line for line in out.splitlines(True)
                                 if not line.startswith("OpenFX: loading")))
        print("exit status %d" % proc.returncode)
        return proc.returncode, out

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


def text_without_save_date(path):
    """Returns the lines of a text project, without its save date."""
    with open(path, "rb") as f:
        lines = f.read().decode("utf-8").splitlines()
    out = []
    skip_value = False
    for line in lines:
        if "<Name>lastSaveDate</Name>" in line:
            skip_value = True
        elif skip_value and "<Value>" in line:
            line = "<Value>(save date)</Value>"
            skip_value = False
        out.append(line)
    return out


def write_variant(data, old, new, path):
    """Writes data, with its first old replaced by new, to path."""
    if old not in data:
        raise ValueError("%r is not in the test project" % (old,))
    with open(path, "wb") as f:
        f.write(data.replace(old, new, 1))


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
        shutil.copyfile(PROJECT, self.src)
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

    def check_no_lock(self, what):
        """No project lock file (<project>.lock) may be left behind."""
        locks = glob.glob(self.path("*.lock"))
        return self.check(not locks, "%s: %s left behind" % (what, locks))

    def check_converted(self, result, dst, to, what):
        """The conversion must succeed and write dst in the format to."""
        status, _ = result
        return (self.check(status == 0, "%s: exit status %d, expected 0"
                           % (what, status)) and
                self.check_no_prompt(result, what) and
                self.check_no_lock(what) and
                self.check(file_format(dst) == to,
                           "%s: %s is not a %s project" % (what, dst, to)))

    def check_refused(self, result, dst, error, what):
        """The conversion must fail with the exception line error."""
        status, out = result
        self.check(status == 1, "%s: exit status %d, expected 1"
                   % (what, status))
        self.check_no_prompt(result, what)
        self.check_no_lock(what)
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

    def test_import(self):
        """Importing the module neither converts nor quits."""
        status, out = self.runner.run(
            ["-t", "-c", "import natron_convert_project; "
             "from NatronEngine import natron; natron.quit(5)"])
        self.check(status == 5, "import: exit status %d, expected 5" % status)
        self.check_no_prompt((status, out), "import")

    def test_chain(self):
        """text -> binary -> text -> binary -> text, same nodes and values."""
        reference = self.dump(self.src)
        if not self.check(reference and "Group1" in reference["nodes"] and
                          "Dot1" in reference["nodes"],
                          "the reference project has no Group1 or Dot1"):
            return
        prev = self.src
        for to, name in (("binary", "1.bin.ntp"), ("text", "2.text.ntp"),
                         ("binary", "3.bin.ntp"), ("text", "4.text.ntp")):
            dst = self.path(name)
            if not self.check_converted(self.runner.convert(prev, dst, to),
                                        dst, to, "%s -> %s" % (prev, to)):
                return
            self.check(self.dump(dst) == reference,
                       "%s does not have the nodes and values of the original"
                       % dst)
            prev = dst
        self.check(text_without_save_date(self.path("2.text.ntp")) ==
                   text_without_save_date(self.path("4.text.ntp")),
                   "2.text.ntp and 4.text.ntp differ (other than the save "
                   "date)")

    def test_in_place(self):
        """Without dst, src is replaced."""
        in_place = self.path("inplace.ntp")
        shutil.copyfile(self.src, in_place)
        self.check_converted(self.runner.convert(in_place, to="binary"),
                             in_place, "binary", "in-place conversion")
        # convert() does not quit: the -t prompt then runs the commands of
        # its standard input (which also checks that the test sees them)
        status, out = self.runner.run(
            ["-t", "-c", "import natron_convert_project as c; "
             "c.convert(%r)" % in_place])
        self.check(status == 0 and PROMPT_MARKER in out and
                   file_format(in_place) == "text",
                   "convert() without quitting: exit status %d, the prompt "
                   "should start and %s should be text" % (status, in_place))

    def test_gui_data(self):
        """A project with GUI data is converted only with discard_gui."""
        # Only the flag: NatronRenderer does not read the GUI part
        with_gui = self.path("withgui.ntp")
        write_variant(self.data, BACKGROUND_TAG,
                      b"<Background_project>0</Background_project>", with_gui)
        dst = self.path("withgui.bin.ntp")
        self.check_refused(self.runner.convert(with_gui, dst, "binary"), dst,
                           "RuntimeError: %s has GUI data" % with_gui,
                           "a project with GUI data")
        result = self.runner.convert(with_gui, dst, "binary", discard_gui=True)
        if self.check_converted(result, dst, "binary", "discard_gui=True"):
            self.check("the GUI data of %s are not converted" % with_gui
                       in result[1], "discard_gui=True: no warning")

    def test_load_errors(self):
        """A project that loads with errors is not converted."""
        # The project loads without the node whose plug-in is missing, at the
        # top level or in a group
        for tag, name in ((DOT_PLUGIN_TAG, "missingplugin"),
                          (INPUT_PLUGIN_TAG, "missingplugingroup")):
            missing = self.path(name + ".ntp")
            write_variant(self.data, tag, MISSING_PLUGIN_TAG, missing)
            dst = self.path(name + ".bin.ntp")
            self.check_refused(self.runner.convert(missing, dst, "binary"),
                               dst,
                               "RuntimeError: there were errors while "
                               "loading %s" % missing,
                               "%s: a project that loads with errors" % name)

    def test_bad_arguments(self):
        """Bad arguments raise the documented exceptions."""
        dst = self.path("bad.ntp")
        self.check_refused(self.runner.convert(self.src, dst, "xml"), dst,
                           "ValueError: to must be \"text\" or \"binary\", "
                           "not 'xml'", "to='xml'")
        missing = self.path("missing.ntp")
        self.check_refused(self.runner.convert(missing, dst), dst,
                           "FileNotFoundError: %s: no such file" % missing,
                           "a missing input")
        no_dir = self.path(os.path.join("nosuchdir", "out.ntp"))
        self.check_refused(self.runner.convert(self.src, no_dir), no_dir,
                           "RuntimeError: %s could not be saved" % no_dir,
                           "an output in a missing directory")
        a_dir = self.path("adir")
        os.mkdir(a_dir)
        status, out = self.runner.convert(self.src, a_dir)
        # Natron would have renamed the directory to adir.~1~ (a backup)
        self.check(status == 1 and "IsADirectoryError: %s is a directory"
                   % a_dir in out and os.path.isdir(a_dir) and
                   not glob.glob(a_dir + ".~*"),
                   "dst is a directory: exit status %d, or it was moved"
                   % status)


def main(argv):
    """Runs the test; returns the exit status."""
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    tmp_dir = tempfile.mkdtemp(prefix="natron-convert-project-test-")
    try:
        test = ConvertProjectTest(Runner(argv[1], wrapper, tmp_dir), tmp_dir)
        test.test_import()
        test.test_chain()
        test.test_in_place()
        test.test_gui_data()
        test.test_load_errors()
        test.test_bad_arguments()
        if test.failures:
            print("%d failure(s)" % len(test.failures))
            return 1
        print("convert project test passed")
        return 0
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
