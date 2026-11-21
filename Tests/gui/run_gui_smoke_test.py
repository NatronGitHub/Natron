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
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import zlib

SKIP = 77
HERE = os.path.dirname(os.path.abspath(__file__))


def writePNG(path, width=320, height=240):
    """Writes an 8-bit RGB test image: a colour gradient with a white square."""
    rows = []
    for y in range(height):
        row = bytearray([0])  # filter type None
        for x in range(width):
            if width // 3 < x < width // 2 and height // 3 < y < height // 2:
                row += b"\xff\xff\xff"
            else:
                row += bytes((255 * x // width, 255 * y // height, 128))
        rows.append(bytes(row))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(b"".join(rows))))
        f.write(chunk(b"IEND", b""))


def hasDisplay():
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
import os
import NatronEngine
from NatronGui import natron
try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtWidgets

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
    # changing a parameter triggers the auto-save
    roto = app.createNode("fr.inria.built-in.Roto")
    for param in roto.getParams():
        if isinstance(param, NatronEngine.DoubleParam):
            param.set(param.get() + 0.25)
            break
    waitForAutosave()

before = autosaveFiles()
QtCore.QTimer.singleShot(200, start)
"""

WRAPPER = []
# seconds, for each Natron run (NATRON_TEST_RUN_TIMEOUT overrides it, e.g. to test the timeout)
RUN_TIMEOUT = int(os.environ.get("NATRON_TEST_RUN_TIMEOUT", "300"))


def descendants(pid):
    """The pids of all the descendants of pid (needs pgrep, else none)."""
    try:
        out = subprocess.check_output(["pgrep", "-P", str(pid)], stderr=subprocess.DEVNULL)
    except (OSError, subprocess.CalledProcessError):
        return []
    ret = []
    for c in out.split():
        c = int(c)
        ret += descendants(c) + [c]
    return ret


def killTree(p):
    """Kills p, its process group, and all its descendants: under lldb, Natron runs in its
    own process group, below debugserver, so killing the group of the wrapper is not enough."""
    if sys.platform == "win32":
        # no process groups nor SIGKILL: taskkill kills the whole tree
        subprocess.call(["taskkill", "/T", "/F", "/PID", str(p.pid)],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            p.kill()
        except OSError:
            pass
        p.wait()
        return
    # let a wrapper such as natron-lldb.sh kill its inferior first
    try:
        p.terminate()
        p.wait(timeout=10)
    except (OSError, subprocess.TimeoutExpired):
        pass
    pids = descendants(p.pid)
    try:
        os.killpg(p.pid, signal.SIGKILL)
    except OSError:
        pass
    for pid in pids + [p.pid]:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    p.wait()


def run(cmd, env):
    cmd = WRAPPER + cmd
    print("Running: " + " ".join(cmd))
    sys.stdout.flush()
    # in its own process group, so that a timeout kills the wrapper and Natron
    p = subprocess.Popen(cmd, env=env, start_new_session=(sys.platform != "win32"))
    try:
        return p.wait(timeout=RUN_TIMEOUT)
    except subprocess.TimeoutExpired:
        print("timeout: killing Natron and its wrapper")
        killTree(p)
        return None


def checkAutosaves(outDir, mode):
    """The auto-save that appeared while the script ran must have been removed at quit."""
    path = os.path.join(outDir, "autosave_%s.txt" % mode)
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
    natronBin = argv[1]
    keep = "--keep" in argv[2:]
    strict = "--strict-exit" in argv[2:]
    if "--debugger" in argv[2:]:
        WRAPPER[:] = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        WRAPPER[:] = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    if os.environ.get("CI") and os.environ.get("NATRON_GUI_TESTS") != "1":
        print("GUI smoke test skipped in CI (set NATRON_GUI_TESTS=1 to run it)")
        return SKIP
    if not hasDisplay():
        print("GUI smoke test skipped: no display")
        return SKIP

    # Debug builds: no trapped floating point exception (Mesa llvmpipe's threads inherit the trapping and
    # its shaders raise exceptions, #1537), as with Natron --test
    os.environ["NATRON_NO_FP_TRAPS"] = "1"
    outDir = tempfile.mkdtemp(prefix="natron-gui-test-")
    # a private disk cache (and OFX plug-in cache), removed with outDir, unless NATRON_DISK_CACHE_PATH is set:
    # the user's one is shared by every Natron of this version, e.g. a concurrent test run (#1569)
    if not os.environ.get("NATRON_DISK_CACHE_PATH"):
        os.environ["NATRON_DISK_CACHE_PATH"] = os.path.join(outDir, "cache")
    try:
        # 1. natron.quit(exitCode) sets the exit status, and
        # 2. closing the main window doesn't block on "Save changes?"
        # (no OFX plug-ins: no crash at exit, see #1253)
        env = dict(os.environ)
        env.pop("OFX_PLUGIN_PATH", None)
        for mode, expected in (("quit", 42), ("close", 0)):
            quitScript = os.path.join(outDir, "quit_%s.py" % mode)
            with open(quitScript, "w") as f:
                f.write("MODE = %r\nOUTDIR = %r\n" % (mode, outDir) + QUIT_SCRIPT)
            status = run([natronBin, "--no-dialogs", "--no-settings",
                          "--setting", "useStdOFXPluginsLocation=False",
                          "--setting", "autoSaveDelay=1", quitScript], env)
            print("Natron exited with status %s" % status)
            if status != expected:
                print("FAILED: %s should exit with status %d" % ("natron.quit(42)" if mode == "quit" else "closing the main window", expected))
                return 1
            if not checkAutosaves(outDir, mode):
                return 1

        # 3. the smoke test
        image = os.path.join(outDir, "input.png")
        writePNG(image)
        env = dict(os.environ)
        env["NATRON_GUI_TEST_OUTDIR"] = outDir
        env["NATRON_GUI_TEST_IMAGE"] = image
        status = run([natronBin, "--no-dialogs", "--no-settings", os.path.join(HERE, "gui_smoke_test_script.py")], env)
        print("Natron exited with status %s" % status)
        if status is None:
            return 1
        statusFile = os.path.join(outDir, "status.txt")
        scriptStatus = None
        if os.path.isfile(statusFile):
            with open(statusFile) as f:
                scriptStatus = int(f.read().strip())
        if scriptStatus is None:
            print("FAILED: the test script did not finish")
            return 1
        if status != scriptStatus:
            print("WARNING: the test script called natron.quit(%d), but Natron exited with status %d: "
                  "crash at exit, see https://github.com/NatronGitHub/Natron/issues/1253" % (scriptStatus, status))
            if strict:
                return 1
        if scriptStatus == 0:
            for name in ("mainWindow.png", "Viewer1.png"):
                if not os.path.isfile(os.path.join(outDir, name)):
                    print("FAILED: missing screenshot " + name)
                    return 1
            print("GUI smoke test passed")
        if keep:
            print("screenshots kept in " + outDir)
        return scriptStatus
    finally:
        if not keep:
            shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
