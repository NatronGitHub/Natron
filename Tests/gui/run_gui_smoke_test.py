#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
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

"""GUI smoke test: runs the Natron GUI with --no-dialogs.

Usage: run_gui_smoke_test.py /path/to/Natron [--keep] [--strict-exit] [--debugger]

1. Natron runs a script that creates a node (so that the project has unsaved changes) and
   calls natron.quit(42) from a timer, without OFX plug-ins: the exit status must be 42.
2. Same, but the script closes the main window: "Save changes?" must not block (it is
   answered Discard with --no-dialogs) and the exit status must be 0.
3. Natron runs gui_smoke_test_script.py, which creates Read -> Transform -> Viewer1,
   saves screenshots of the main window and of Viewer1 (checking that they are not
   black) and of the other tabs, changes the Transform while the node previews render,
   and calls natron.quit(status). (The overlay drag, undo and viewer shortcuts are
   tested by the --test scripts of #1455.)
   The script also writes its status to a file: with some OFX plug-ins (those using TBB,
   like openfx-io), Natron may crash at exit, after natron.quit() (issue #1253). That
   crash is reported, and only fails the test with --strict-exit.

Each Natron run is limited to 300 seconds; on timeout, its process group is killed.
Runs 1 and 2 also check that the auto-save of the project is removed when quitting.
--debugger runs Natron under lldb with natron-lldb.sh (next to this script): a crash
prints the backtrace of all threads and gives the exit status 139, and macOS does not
show its "quit unexpectedly" dialog. The environment variable NATRON_TEST_WRAPPER may
instead give any command to run Natron with (e.g. "/path/to/natron-lldb.sh" or "valgrind").

Exit status: 0 on success, 77 if skipped (no display, running in CI without
NATRON_GUI_TESTS=1, or the Read/Transform plug-ins are missing: set OFX_PLUGIN_PATH
to the openfx-io and openfx-misc plug-ins), anything else on failure.
The screenshots are written to a temporary directory, deleted unless --keep is given.
It never takes a screen-level screenshot: only Natron's widgets are captured.
"""

import os
import shutil
import subprocess
import sys
import tempfile

import natronrunner


def has_display():
    if sys.platform.startswith("linux") or sys.platform.startswith("freebsd"):
        return bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))
    if sys.platform == "darwin":
        # "Aqua" in a GUI login session; "Background" or "System" e.g. over ssh, where the
        # process cannot reach the WindowServer
        try:
            out = subprocess.check_output(["launchctl", "managername"], stderr=subprocess.DEVNULL)
            return out.decode(errors="replace").strip() == "Aqua"
        except (OSError, subprocess.CalledProcessError):
            return False
    return True  # Windows: assume a logged-in session


# The project has unsaved changes (a node was created) when Natron quits. The script waits
# for the auto-save of the project (--setting autoSaveDelay=1), then quits, so that the
# test can check that the auto-save was removed.
QUIT_SCRIPT = """
import importlib, os, sys
from NatronGui import natron
# the PySide that NatronGui loaded (the one of Natron's Qt)
_pyside = "PySide6" if "PySide6.QtCore" in sys.modules else "PySide2"
QtCore = importlib.import_module(_pyside + ".QtCore")
QtWidgets = importlib.import_module(_pyside + ".QtWidgets")

autosaves = os.path.join(QtCore.QStandardPaths.writableLocation(QtCore.QStandardPaths.AppLocalDataLocation), "Autosaves")
waited = [0]

def autosaveFiles():
    try:
        # .ntp.autosave (text, Natron 2.5) or .ntp.binautosave (binary, 2.6, #1056)
        return [f for f in os.listdir(autosaves) if ".autosave" in f or ".binautosave" in f]
    except OSError:
        return []

def quit():
    if MODE == "close":
        for w in QtWidgets.QApplication.topLevelWidgets():
            if isinstance(w, QtWidgets.QMainWindow) and w.isVisible():
                w.close()  # asks "Save changes?"
                break
    else:
        natron.quit(42)

def waitForAutosave():
    waited[0] += 1
    found = [f for f in autosaveFiles() if f not in before]
    if found or waited[0] > 50:
        with open(os.path.join(OUTDIR, "autosave_%s.txt" % MODE), "w") as f:
            f.write(autosaves + "\\n" + "\\n".join(found) + "\\n")
        quit()
    else:
        QtCore.QTimer.singleShot(200, waitForAutosave)

def start():
    app = natron.getGuiInstance(0)
    app.createNode("fr.inria.built-in.Viewer")
    # changing a parameter triggers the auto-save; this run has no OFX plug-ins, so use a node that needs
    # none (a Roto node needs openfx-misc, #1828)
    dot = app.createNode("fr.inria.built-in.Dot")
    param = dot.createDoubleParam("smokeTestValue", "Smoke test value")
    param.set(param.get() + 0.25)
    waitForAutosave()

before = autosaveFiles()
QtCore.QTimer.singleShot(200, start)
"""

# seconds, for each Natron run (NATRON_TEST_RUN_TIMEOUT overrides it, e.g. to test the timeout)
RUN_TIMEOUT = int(os.environ.get("NATRON_TEST_RUN_TIMEOUT", "300"))


def run(cmd, env):
    """Runs cmd (its output is not captured); returns its exit status, or None after a timeout."""
    print("Running: " + " ".join(cmd))
    sys.stdout.flush()
    # in its own process group, so that a timeout kills the wrapper and Natron
    p = subprocess.Popen(cmd, env=env, start_new_session=(sys.platform != "win32"))
    try:
        return p.wait(timeout=RUN_TIMEOUT)
    except subprocess.TimeoutExpired:
        print("timeout: killing Natron and its wrapper")
        natronrunner.kill_tree(p)
        return None


def check_autosaves(out_dir, mode):
    """The auto-save that appeared while the script ran must have been removed at quit."""
    path = os.path.join(out_dir, "autosave_%s.txt" % mode)
    if not os.path.isfile(path):
        print("FAILED: the %s script did not finish" % mode)
        return False
    with open(path) as f:
        lines = [l for l in f.read().split("\n") if l]
    autosaves, found = lines[0], lines[1:]
    if not found:
        print("note: no auto-save was written before quitting (%s)" % mode)
    left = [f for f in found if os.path.exists(os.path.join(autosaves, f))]
    if left:
        print("FAILED: auto-saves left in %s after quitting (%s): %s" % (autosaves, mode, ", ".join(left)))
        return False
    return True


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    natron_bin = natronrunner.wrapper(argv[2:]) + [argv[1]]
    keep = "--keep" in argv[2:]
    strict = "--strict-exit" in argv[2:]
    if os.environ.get("CI") and os.environ.get("NATRON_GUI_TESTS") != "1":
        print("GUI smoke test skipped in CI (set NATRON_GUI_TESTS=1 to run it)")
        return natronrunner.SKIP
    if not has_display():
        print("GUI smoke test skipped: no display")
        return natronrunner.SKIP

    # Debug builds: no trapped floating point exception (Mesa llvmpipe's threads inherit the trapping and
    # its shaders raise exceptions, #1537), as with Natron --test
    os.environ["NATRON_NO_FP_TRAPS"] = "1"
    out_dir = tempfile.mkdtemp(prefix="natron-gui-test-")
    # a private disk cache (and OFX plug-in cache), removed with out_dir, unless NATRON_DISK_CACHE_PATH is set:
    # the user's one is shared by every Natron of this version, e.g. a concurrent test run (#1569)
    if not os.environ.get("NATRON_DISK_CACHE_PATH"):
        os.environ["NATRON_DISK_CACHE_PATH"] = os.path.join(out_dir, "cache")
    try:
        # 1. natron.quit(exitCode) sets the exit status, and
        # 2. closing the main window doesn't block on "Save changes?"
        # (no OFX plug-ins: no crash at exit, see #1253)
        env = dict(os.environ)
        env.pop("OFX_PLUGIN_PATH", None)
        for mode, expected in (("quit", 42), ("close", 0)):
            quit_script = os.path.join(out_dir, "quit_%s.py" % mode)
            with open(quit_script, "w") as f:
                f.write("MODE = %r\nOUTDIR = %r\n" % (mode, out_dir) + QUIT_SCRIPT)
            status = run(natron_bin + ["--no-dialogs", "--no-settings",
                                       "--setting", "useStdOFXPluginsLocation=False",
                                       "--setting", "autoSaveDelay=1", quit_script], env)
            print("Natron exited with status %s" % status)
            if status != expected:
                print("FAILED: %s should exit with status %d" % ("natron.quit(42)" if mode == "quit" else "closing the main window", expected))
                return 1
            if not check_autosaves(out_dir, mode):
                return 1

        # 3. the smoke test
        image = os.path.join(out_dir, "input.png")
        natronrunner.write_png(image)
        env = dict(os.environ)
        env["NATRON_GUI_TEST_OUTDIR"] = out_dir
        env["NATRON_GUI_TEST_IMAGE"] = image
        script = os.path.join(natronrunner.HERE, "gui_smoke_test_script.py")
        status = run(natron_bin + ["--no-dialogs", "--no-settings", script], env)
        print("Natron exited with status %s" % status)
        if status is None:
            return 1
        status_file = os.path.join(out_dir, "status.txt")
        script_status = None
        if os.path.isfile(status_file):
            with open(status_file) as f:
                script_status = int(f.read().strip())
        if script_status is None:
            print("FAILED: the test script did not finish")
            return 1
        if status != script_status:
            print("WARNING: the test script called natron.quit(%d), but Natron exited with status %d: "
                  "crash at exit, see https://github.com/NatronGitHub/Natron/issues/1253" % (script_status, status))
            if strict:
                return 1
        if script_status == 0:
            for name in ("mainWindow.png", "Viewer1.png"):
                if not os.path.isfile(os.path.join(out_dir, name)):
                    print("FAILED: missing screenshot " + name)
                    return 1
            print("GUI smoke test passed")
        elif script_status == natronrunner.SKIP:
            print("GUI smoke test: runs 1 and 2 passed, run 3 (the smoke test script) skipped")
        if keep:
            print("screenshots kept in " + out_dir)
        return script_status
    finally:
        if not keep:
            shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
