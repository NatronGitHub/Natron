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
   black), drags the Transform overlay handle, undoes it, sends viewer shortcuts, and
   calls natron.quit(status).
   The script also writes its status to a file: with some OFX plug-ins (those using TBB,
   like openfx-io), Natron may crash at exit, after natron.quit() (issue #1253). That
   crash is reported, and only fails the test with --strict-exit.

Each Natron run is limited to 300 seconds.
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
    return True  # macOS and Windows: assume a logged-in session


# The project has unsaved changes (a node was created) when Natron quits
QUIT_SCRIPT = """
from NatronGui import natron
try:
    from PySide2 import QtCore, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtWidgets

def quit():
    natron.getGuiInstance(0).createNode("fr.inria.built-in.Viewer")
    if MODE == "close":
        for w in QtWidgets.QApplication.topLevelWidgets():
            if isinstance(w, QtWidgets.QMainWindow) and w.isVisible():
                w.close()  # asks "Save changes?"
                break
    else:
        natron.quit(42)

QtCore.QTimer.singleShot(200, quit)
"""

WRAPPER = []


def run(cmd, env):
    cmd = WRAPPER + cmd
    print("Running: " + " ".join(cmd))
    sys.stdout.flush()
    try:
        return subprocess.call(cmd, env=env, timeout=300)
    except subprocess.TimeoutExpired:
        print("timeout")
        return None


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

    outDir = tempfile.mkdtemp(prefix="natron-gui-test-")
    try:
        # 1. natron.quit(exitCode) sets the exit status, and
        # 2. closing the main window doesn't block on "Save changes?"
        # (no OFX plug-ins: no crash at exit, see #1253)
        env = dict(os.environ)
        env.pop("OFX_PLUGIN_PATH", None)
        for mode, expected in (("quit", 42), ("close", 0)):
            quitScript = os.path.join(outDir, "quit_%s.py" % mode)
            with open(quitScript, "w") as f:
                f.write("MODE = %r\n" % mode + QUIT_SCRIPT)
            status = run([natronBin, "--no-dialogs", "--no-settings",
                          "--setting", "useStdOFXPluginsLocation=False", quitScript], env)
            print("Natron exited with status %s" % status)
            if status != expected:
                print("FAILED: %s should exit with status %d" % ("natron.quit(42)" if mode == "quit" else "closing the main window", expected))
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
