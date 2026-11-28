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

"""Checks that the Python project format keeps everything the .ntp keeps (#2001).

Usage: run_ntp_python_roundtrip_test.py /path/to/NatronRenderer [options] [project...]

For each project P (.ntp, text or binary, or .py), three NatronRenderer runs,
each in a new process:
1. load P and save it as a text .ntp (A): saving after loading normalises it;
2. load P and save it as Python (P.py, App.saveProjectAsPython());
3. load P.py and save it as a text .ntp (B).
Text saves are deterministic (the ProjectFileFormat.RoundTrip test), so A and
B must be the same. They are compared as XML trees: nodes and parameters by
name, other list items by position, numbers by value. Only what must differ
is ignored (see IGNORED_TAGS, IGNORED_PATHS and _normalize_value()). Every
other difference fails, unless the allowlist has it: each entry names the
issue that tracks the loss (see ntp_python_roundtrip_allowlist.txt).

A project that doesn't load, or loads with errors (a missing plug-in, for
example), is skipped, with the reason. A Python project that doesn't load is
the difference "/(PythonProjectLoad)".

Without a project (or with --builtin): the built-in corpus, which needs no
OpenFX plug-in: the project that run_project_python_test.py builds (as text
.ntp and as Python), and the projects of Tests/data.

Options:
  --debugger         run NatronRenderer under natron-lldb.sh (or set
                     NATRON_TEST_WRAPPER, see natronrunner.py)
  --builtin          also check the built-in corpus
  --projects-from FILE  also check the projects listed in FILE, one per line
  --allowlist FILE   the allowlist (default: ntp_python_roundtrip_allowlist.txt
                     next to this script)
  --next-to-project  write P.py next to P (and remove it), so that [Project]
                     is the same folder: the file paths of a project such as
                     [Project]/image.png stay valid. Default: a temporary folder.
                     Not for the built-in corpus, whose files are not written to.
  --jobs N           run N projects at a time (default 1)
  --keep DIR         keep A, B and P.py of each project with differences in DIR
  --report-unused    list the allowlist entries that no difference used

Exit status: 0 if every project that loaded has no difference but the
allowed ones, 1 otherwise, 77 (skip) if no project loaded, 2 on a usage error.
"""

import argparse
import concurrent.futures
import dataclasses
import fnmatch
import os
import queue
import re
import shutil
import sys
import tempfile
import threading
import xml.etree.ElementTree as ET

import natronrunner
import run_project_python_test

TIMEOUT = 600  # seconds, for each run of NatronRenderer (BayMax takes the longest)
DATA_DIR = os.path.join(os.path.dirname(natronrunner.HERE), "data")
DEFAULT_ALLOWLIST = os.path.join(natronrunner.HERE, "ntp_python_roundtrip_allowlist.txt")
# The first or second line of a Python project (NATRON_PROJECT_PYTHON_MAGIC)
PYTHON_MAGIC = b"# Natron Project"
# The difference of a project whose Python project doesn't load (step 3)
LOAD_FAILURE_PATH = "/(PythonProjectLoad)"
# The exit status of SAVE when the project loaded with errors
LOAD_ERRORS = 3

# Loads RT_INPUT and saves it to RT_OUTPUT, as a text .ntp or as Python (RT_FORMAT).
SAVE = r'''
import os
import traceback

import NatronEngine
from NatronEngine import natron

status = 1
try:
    project = natron.getInstance(0).loadProject(os.environ["RT_INPUT"])
    if project is None:
        raise RuntimeError("app.loadProject() failed")
    if project.projectLoadedWithErrors():
        print("ROUNDTRIP: the project loaded with errors")
        status = %d
    else:
        output = os.environ["RT_OUTPUT"]
        if os.environ["RT_FORMAT"] == "python":
            saved = project.saveProjectAsPython(output)
        else:
            saved = project.saveProjectCopy(
                output, NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatText)
        if not saved:
            raise RuntimeError("could not save " + output)
        status = 0
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
''' % LOAD_ERRORS

# Elements that only repeat what the comparison checks: the counts of the
# lists, whose items are compared
IGNORED_TAGS = frozenset((
    "count", "item_version", "NodesCount", "KnobsCount", "ProjectKnobsCount",
    "UserPagesCount", "PagesCount", "NbChildren", "Children",
))
# Elements that must differ: the creation time of the project, the default
# of its creationDate parameter (the time the session started), and what each
# save sets (Project::saveProjectInternal()): the date and the author
IGNORED_PATHS = frozenset((
    "/Project/CreationDate", "/Project/{creationDate}/#0/Default",
    "/Project/{lastSaveDate}", "/Project/{lastAuthor}",
))
# The elements whose text names their list item (a node, a parameter)
KEY_TAGS = ("Plugin_script_name", "Name")
# The [Project] entry of the project paths: the folder of the file, which the
# project gets when it is loaded or saved (so it isn't always at the same place)
PROJECT_PATH_RE = re.compile(r"<Name>Project</Name><Value>[^<]*</Value>")


@dataclasses.dataclass
class AllowlistEntry:
    """A line of the allowlist, and how many differences it allowed."""
    issue: str
    path_pattern: str
    project_pattern: str
    uses: int = 0


class Allowlist:
    """The accepted differences, each with the issue that tracks it.

    Each line of the file: an issue (#N or owner/repo#N), a pattern of the
    path of a difference, and optionally a pattern of the project (its label:
    the path given on the command line, with forward slashes); the patterns
    are shell patterns (fnmatch: * also matches /). Lines that start with
    "# " are comments (a path has "#": no comment at the end of a line).
    """

    ISSUE_RE = re.compile(r"^([\w.-]+/[\w.-]+)?#[0-9]+$")

    def __init__(self, path):
        self.entries = []  # AllowlistEntry
        self._lock = threading.Lock()  # guards the uses of the entries (the jobs are threads)
        if not os.path.exists(path):
            return
        with open(path, encoding="utf-8") as f:
            for number, line in enumerate(f, 1):
                fields = line.split()
                if not fields or fields[0] == "#" or line.lstrip().startswith("# "):
                    continue
                if len(fields) not in (2, 3) or not self.ISSUE_RE.match(fields[0]):
                    raise ValueError("%s:%d: expected '<issue> <path pattern> [<project pattern>]'"
                                     % (path, number))
                self.entries.append(AllowlistEntry(*fields) if len(fields) == 3 else AllowlistEntry(*fields, "*"))

    def issue(self, project, difference_path):
        """The issue of the first entry that allows the difference, or None."""
        project = project.replace("\\", "/")
        with self._lock:
            return self._issue(project, difference_path)

    def _issue(self, project, difference_path):
        for entry in self.entries:
            if (fnmatch.fnmatchcase(difference_path, entry.path_pattern)
                    and fnmatch.fnmatchcase(project, entry.project_pattern)):
                entry.uses += 1
                return entry.issue
        return None


def _normalize_value(tag, text):
    """The text of an element, without what must differ."""
    text = (text or "").strip()
    if tag == "Value" and "<Name>Project</Name>" in text:
        text = PROJECT_PATH_RE.sub("", text)
    return text


def _same_value(a, b):
    """True if two texts are the same, or the same number."""
    if a == b:
        return True
    try:
        return float(a) == float(b)
    except ValueError:
        return False


def _key(element):
    """The name of a list item (a node, a parameter), or None."""
    for tag in KEY_TAGS:
        child = element.find(tag)
        if child is not None and child.text:
            return child.text.strip()
    return None


def _children(element, parent_path):
    """The children of element, by path segment: {segment: (path, child)}.

    Items with a name are keyed by it ({name}, {name}(user) for a user
    parameter, which the exporter writes in full), other items by their position
    (#n), other elements by their tag (with #n if the tag is repeated). The
    Value of an animated dimension is "Value(animated)": it is the value at
    the current frame, which only depends on the curve and on that frame.
    """
    animated = element.tag == "item" and element.findtext("HasAnimation") == "1"
    ret = {}
    positions = {}
    for child in element:
        if child.tag in IGNORED_TAGS:
            continue
        key = _key(child) if child.tag == "item" else None
        if key is not None:
            segment = "{%s}" % key
            if child.findtext("UserKnob") == "1":
                segment += "(user)"
        else:
            positions[child.tag] = positions.get(child.tag, -1) + 1
            segment = child.tag if child.tag != "item" else ""
            segment += "#%d" % positions[child.tag] if (child.tag == "item" or positions[child.tag]) else ""
            if animated and child.tag == "Value":
                segment = "Value(animated)"
        while segment in ret:  # two items with the same name
            segment += "'"
        path = "%s/%s" % (parent_path, segment)
        if path not in IGNORED_PATHS:
            ret[segment] = (path, child)
    return ret


def compare(a, b, path=""):
    """The differences between two XML elements, recursively.

    Items with a name are matched by name: the order of nodes, of parameters,
    and of the parameters of a user page, is not compared (the page order is:
    the "name" items of a node).

    Returns:
      A list of (path, description), e.g.
      ("/Project/NodesCollection/{Grp}/{amount}/#0/Value", "A '1.5', B '2'").
    """
    differences = []
    text_a = _normalize_value(a.tag, a.text)
    text_b = _normalize_value(b.tag, b.text)
    if not _same_value(text_a, text_b):
        differences.append((path, "A %r, B %r" % (_shorten(text_a), _shorten(text_b))))
    children_a = _children(a, path)
    children_b = _children(b, path)
    for segment, (child_path, child) in children_a.items():
        if segment not in children_b:
            differences += _only_in("A", child, child_path)
        else:
            differences += compare(child, children_b[segment][1], child_path)
    for segment, (child_path, child) in children_b.items():
        if segment not in children_a:
            differences += _only_in("B", child, child_path)
    return differences


def _only_in(side, element, path):
    """The differences of an element that only one file has.

    A user parameter is one difference, at its path. Otherwise, a file has
    only the parameters that have changes (see
    KnobHelper::hasModificationsForSerialization()): for a parameter, the
    changes it has, each at its path (a value that isn't the default, a
    disabled or secret parameter, an animation, an expression, a link), so
    that an allowlist entry for one of them doesn't allow the others.
    """
    if element.tag != "item" or element.find("Name") is None or element.find("Type") is None:
        return [(path, "only in %s%s" % (side, _summary(element)))]
    if element.findtext("UserKnob") == "1":
        # the exporter creates the user parameters: a missing one is lost, whatever its state
        return [(path, "only in %s: a user parameter%s" % (side, _summary(element)))]
    differences = []
    secret = element.find("Secret")
    if secret is not None and secret.text == "1":
        differences.append((path + "/Secret", "only in %s: secret" % side))
    for d, dimension in enumerate(element.findall("item")):
        changes = []
        if dimension.findtext("Enabled") == "0":
            changes.append(("Enabled", "disabled"))
        value, default = dimension.findtext("Value"), dimension.findtext("Default")
        if value is not None and default is not None and not _same_value(value.strip(), default.strip()):
            changes.append(("Value", "%r, default %r" % (_shorten(value.strip()), _shorten(default.strip()))))
        if dimension.findtext("HasAnimation") == "1":
            changes.append(("Curve", "animated"))
        if (dimension.findtext("Expression") or "").strip():
            changes.append(("Expression", repr(_shorten(dimension.findtext("Expression").strip()))))
        if dimension.findtext("HasMaster") == "1":
            changes.append(("HasMaster", "linked"))
        for tag, description in changes:
            differences.append(("%s/#%d/%s" % (path, d, tag), "only in %s: %s" % (side, description)))
    if not differences:
        differences.append((path, "only in %s%s" % (side, _summary(element))))
    return differences


# A check of compare(), on two small files: nodes and parameters in another
# order, the same numbers written differently, and three losses
SELF_TEST_A = """<Project><NodesCollection>
<item><Plugin_script_name>Dot1</Plugin_script_name><KnobsCount>2</KnobsCount>
<item><Name>a</Name><Type>Double</Type><Secret>0</Secret><item><Enabled>1</Enabled><Value>1</Value><Default>0</Default></item></item>
<item><Name>b</Name><Type>Double</Type><Secret>1</Secret><item><Enabled>1</Enabled><Value>0</Value><Default>0</Default></item></item>
</item>
<item><Plugin_script_name>Dot2</Plugin_script_name><Inputs_map><item><first>0</first><second>Dot1</second></item></Inputs_map></item>
</NodesCollection></Project>"""
SELF_TEST_B = """<Project><NodesCollection>
<item><Plugin_script_name>Dot2</Plugin_script_name><Inputs_map><item><first>0</first><second>Dot3</second></item></Inputs_map></item>
<item><Plugin_script_name>Dot1</Plugin_script_name><KnobsCount>1</KnobsCount>
<item><Name>a</Name><Type>Double</Type><Secret>0</Secret><item><Enabled>1</Enabled><Value>1.0e+00</Value><Default>0</Default></item></item>
</item>
<item><Plugin_script_name>Dot3</Plugin_script_name></item>
</NodesCollection></Project>"""
SELF_TEST_DIFFERENCES = [
    ("/NodesCollection/{Dot1}/{b}/Secret", "only in A: secret"),
    ("/NodesCollection/{Dot2}/Inputs_map/#0/second", "A 'Dot1', B 'Dot3'"),
    ("/NodesCollection/{Dot3}", "only in B"),
]


def _check_compare():
    """True if compare() finds the differences of SELF_TEST_A and SELF_TEST_B."""
    got = sorted(compare(ET.fromstring(SELF_TEST_A), ET.fromstring(SELF_TEST_B)))
    if got == SELF_TEST_DIFFERENCES:
        return True
    print("FAILED: compare() found %r instead of %r" % (got, SELF_TEST_DIFFERENCES))
    return False


def _shorten(text, length=200):
    return text if len(text) <= length else text[:length] + "..."


def _summary(element):
    """A short description of an element that only one file has."""
    if len(element) == 0:
        return ": %r" % _shorten(_normalize_value(element.tag, element.text))
    values = [_shorten(_normalize_value(e.tag, e.text), 40) for e in element.iter("Value")]
    return " (values %s)" % ", ".join(repr(v) for v in values[:8]) if values else ""


def _run(renderer, env, script, source, output, fmt):
    """Runs SAVE in a new NatronRenderer process.

    Returns:
      (status, output): status is "ok", "load errors" or a failure message.
    """
    env = dict(env, RT_INPUT=source, RT_OUTPUT=output, RT_FORMAT=fmt)
    status, log = natronrunner.run(renderer + ["-t", script], env, TIMEOUT)
    if status == 0 and os.path.isfile(output):
        return "ok", log
    if status == LOAD_ERRORS:
        return "load errors", log
    if status is None:
        return "timeout", log
    return "exit status %d" % status, log


def check_project(project, label, next_to_project, renderer, env, script, work_dir, allowlist, keep):
    """Runs the three steps on one project and compares A and B.

    Args:
      project: the path of P.
      label: its name in the report, and for the allowlist.
      next_to_project: write P.py next to P (see --next-to-project).
      renderer: the NatronRenderer command.
      env: its environment.
      script: the path of SAVE.
      work_dir: a new folder for A and B (and P.py).
      allowlist: the Allowlist.
      keep: the folder where A, B and P.py are copied if they differ, or None.

    Returns:
      (result, report): result is "pass", "fail" or "skip".
    """
    os.makedirs(work_dir)
    lines = ["*** %s" % label]
    file_a = os.path.join(work_dir, "A.ntp")
    file_b = os.path.join(work_dir, "B.ntp")
    if next_to_project:
        stem = os.path.splitext(project)[0]
        python_file = stem + ".ntp-python-roundtrip.py"
    else:
        python_file = os.path.join(work_dir, "P.py")
    try:
        differences = None
        for source, output, fmt in ((project, file_a, "text"), (project, python_file, "python"),
                                    (python_file, file_b, "text")):
            status, log = _run(renderer, env, script, source, output, fmt)
            if status != "ok":
                lines.append(log)
                if status == "load errors" and source == project:
                    lines.append("SKIP: %s loads with errors" % label)
                    return "skip", lines
                if source != python_file:
                    lines.append("FAIL: %s: loading %s and saving it as %s: %s"
                                 % (label, os.path.basename(source), fmt, status))
                    return "fail", lines
                # the Python project doesn't load: a loss, which the allowlist may have
                differences = [(LOAD_FAILURE_PATH, "the Python project didn't load: %s" % status)]
        try:
            if differences is None:
                differences = compare(ET.parse(file_a).getroot(), ET.parse(file_b).getroot())
        except ET.ParseError as e:
            lines.append("FAIL: %s: %s" % (label, e))
            return "fail", lines
        failed = False
        for path, description in differences:
            issue = allowlist.issue(label, path)
            if issue:
                lines.append("  allowed (%s): %s: %s" % (issue, path, description))
            else:
                lines.append("  %s: %s" % (path, description))
                failed = True
        if failed and keep:
            keep_dir = os.path.join(keep, re.sub(r"[^\w.-]", "_", label))
            os.makedirs(keep_dir, exist_ok=True)
            for f in (file_a, file_b, python_file):
                if os.path.exists(f):
                    shutil.copy(f, keep_dir)
        lines.append("%s: %s" % ("FAIL" if failed else "PASS", label))
        return ("fail" if failed else "pass"), lines
    finally:
        if next_to_project and os.path.exists(python_file):
            os.remove(python_file)


def _is_python_project(path):
    """True if path is a Python project (not a script that makes one)."""
    with open(path, "rb") as f:
        return any(line.strip() == PYTHON_MAGIC for line in f.read(256).splitlines()[:2])


def _builtin_corpus(renderer, env, tmp_dir):
    """Builds the project of run_project_python_test.py; returns the built-in corpus.

    Returns:
      [(path, label, False)], or None if the project could not be built.
    """
    built_dir = os.path.join(tmp_dir, "built")
    os.mkdir(built_dir)
    files = run_project_python_test.write_test_files(built_dir, env)
    if not run_project_python_test.run_renderer(renderer + ["-t", files["build"]], env):
        return None
    corpus = [(files["textntp"], "ProjectPythonTest.ntp", False), (files["first"], "ProjectPythonTest.py", False)]
    for name in sorted(os.listdir(DATA_DIR)):
        path = os.path.join(DATA_DIR, name)
        if name.endswith(".ntp") or (name.endswith(".py") and _is_python_project(path)):
            corpus.append((path, "Tests/data/" + name, False))
    return corpus


def _parse_args(argv):
    parser = argparse.ArgumentParser(usage=__doc__.split("\n\n", 2)[1])
    parser.add_argument("renderer")
    parser.add_argument("projects", nargs="*")
    parser.add_argument("--debugger", action="store_true")
    parser.add_argument("--builtin", action="store_true")
    parser.add_argument("--projects-from")
    parser.add_argument("--allowlist", default=DEFAULT_ALLOWLIST)
    parser.add_argument("--next-to-project", action="store_true")
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--keep")
    parser.add_argument("--report-unused", action="store_true")
    return parser.parse_args(argv)


def main(argv):
    """Runs the check, see the module docstring.

    Returns:
      The exit status.
    """
    options = _parse_args(argv[1:])
    if not _check_compare():
        return 1
    allowlist = Allowlist(options.allowlist)
    renderer = natronrunner.wrapper(["--debugger"] if options.debugger else []) + [
        options.renderer, "--no-settings"]
    tmp_dir = os.path.realpath(tempfile.mkdtemp(prefix="natron-ntp-python-roundtrip-"))
    try:
        env = natronrunner.natron_env(tmp_dir)
        script = os.path.join(tmp_dir, "save.py")
        with open(script, "w", encoding="utf-8") as f:
            f.write(SAVE)
        projects = list(options.projects)
        if options.projects_from:
            with open(options.projects_from, encoding="utf-8") as f:
                projects += [line.strip() for line in f if line.strip()]
        corpus = []
        if options.builtin or not projects:
            corpus = _builtin_corpus(renderer, env, tmp_dir)
            if corpus is None:
                print("FAILED: could not build the project of run_project_python_test.py")
                return 1
        corpus += [(os.path.abspath(p), p, options.next_to_project) for p in projects]
        # one disk cache (and OpenFX plug-in cache) per job: concurrent Natrons must not share one (#1569)
        caches = queue.Queue()
        for i in range(max(1, options.jobs)):
            caches.put(os.path.join(tmp_dir, "cache%d" % i))

        def run_one(index):
            cache = caches.get()
            try:
                project, label, next_to_project = corpus[index]
                return check_project(project, label, next_to_project, renderer,
                                     dict(env, NATRON_DISK_CACHE_PATH=cache), script,
                                     os.path.join(tmp_dir, "p%d" % index), allowlist, options.keep)
            finally:
                caches.put(cache)

        results = {"pass": [], "fail": [], "skip": []}
        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, options.jobs)) as pool:
            for (_, label, _), (result, lines) in zip(corpus, pool.map(run_one, range(len(corpus)))):
                print("\n".join(lines))
                sys.stdout.flush()
                results[result].append(label)
        print("\n*** %d passed, %d failed, %d skipped" % tuple(len(results[r]) for r in ("pass", "fail", "skip")))
        for result in ("fail", "skip"):
            for label in results[result]:
                print("%s: %s" % (result.upper(), label))
        unused = [entry for entry in allowlist.entries if not entry.uses]
        print("%d allowlist entries not used (fixed, or not in this corpus)%s"
              % (len(unused), ":" if unused and options.report_unused else ""))
        if options.report_unused:
            for entry in unused:
                print("  not used: %s %s %s" % (entry.issue, entry.path_pattern, entry.project_pattern))
        if results["fail"]:
            return 1
        if not results["pass"]:
            print("SKIPPED: no project loaded")
            return natronrunner.SKIP
        return 0
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
