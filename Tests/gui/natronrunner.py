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

"""Helpers shared by the test drivers of Tests/gui (run_*.py), which ctest runs with the host Python.

write_png() is also used by GUI tests, inside Natron.

A driver starts Natron or NatronRenderer (the path is its first argument), checks the exit status
and the output, and exits with 0 (pass), SKIP (77, skipped) or 1 (fail). The option --debugger
runs Natron under natron-lldb.sh, which prints a backtrace on a crash; the environment variable
NATRON_TEST_WRAPPER gives another wrapper command (e.g. "valgrind -q").
"""

import os
import shlex
import signal
import struct
import subprocess
import sys
import zlib

SKIP = 77
HERE = os.path.dirname(os.path.abspath(__file__))


def wrapper(options):
    """The command that runs Natron, from the driver's options: natron-lldb.sh for --debugger."""
    if "--debugger" in options:
        return [os.path.join(HERE, "natron-lldb.sh")]
    if os.environ.get("NATRON_TEST_WRAPPER"):
        return shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    return []


def natron_env(out_dir, **variables):
    """The environment of a Natron run: a copy of os.environ, with variables added (None: removed).

    The disk cache (and the OFX plug-in cache) is in out_dir unless NATRON_DISK_CACHE_PATH is set: the
    user's one is shared by every Natron of this version, e.g. a concurrent test run (#1569). Debug
    builds trap no floating point exception (#1537), as with Natron --test.
    """
    env = dict(os.environ)
    if not env.get("NATRON_DISK_CACHE_PATH"):
        env["NATRON_DISK_CACHE_PATH"] = os.path.join(out_dir, "cache")
    env["NATRON_NO_FP_TRAPS"] = "1"
    for name, value in variables.items():
        if value is None:
            env.pop(name, None)
        else:
            env[name] = value
    return env


def _descendants(pid):
    """The pids of all the descendants of pid (needs pgrep, else none)."""
    try:
        out = subprocess.check_output(["pgrep", "-P", str(pid)], stderr=subprocess.DEVNULL)
    except (OSError, subprocess.CalledProcessError):
        return []
    ret = []
    for child in out.split():
        child = int(child)
        ret += _descendants(child) + [child]
    return ret


def kill_tree(proc):
    """Kills proc, its process group, and all its descendants.

    Under lldb, Natron runs in its own process group, below debugserver, so killing the group of
    the wrapper is not enough.
    """
    if sys.platform == "win32":
        # no process groups nor SIGKILL: taskkill kills the whole tree
        subprocess.call(["taskkill", "/T", "/F", "/PID", str(proc.pid)],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            proc.kill()
        except OSError:
            pass
        proc.wait()
        return
    # before the wrapper exits: its children are then no longer its descendants
    pids = _descendants(proc.pid)
    # let a wrapper such as natron-lldb.sh kill its inferior first
    try:
        proc.terminate()
        proc.wait(timeout=10)
    except (OSError, subprocess.TimeoutExpired):
        pass
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except OSError:
        pass
    for pid in pids + [proc.pid]:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    proc.wait()


def run(cmd, env, timeout, stdin=None):
    """Runs cmd and returns (exit status, output), or (None, output) after a timeout.

    The output is stdout and stderr, decoded, with "\\n" line ends. stdin is the path of a file to read
    stdin from (natron-lldb.sh reads it from NATRON_LLDB_STDIN), or None for the null device. The
    command runs in its own process group: on a timeout, kill_tree() kills it with its wrapper.
    """
    print("Running: " + " ".join(cmd) + (" < " + stdin if stdin else ""))
    sys.stdout.flush()
    if stdin:
        env = dict(env, NATRON_LLDB_STDIN=stdin)
    with open(stdin, "rb") if stdin else open(os.devnull, "rb") as stdin_file:
        proc = subprocess.Popen(cmd, env=env, stdin=stdin_file, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                start_new_session=(sys.platform != "win32"))
        try:
            out, _ = proc.communicate(timeout=timeout)
            status = proc.returncode
        except subprocess.TimeoutExpired:
            print("timeout after %g s: killing the process and its wrapper" % timeout)
            # a wrapper such as natron-lldb.sh kills its inferior and prints its log: keep reading it
            proc.terminate()
            try:
                out, _ = proc.communicate(timeout=10)
            except subprocess.TimeoutExpired:
                kill_tree(proc)
                out, _ = proc.communicate()
            status = None
    return status, out.decode(errors="replace").replace("\r\n", "\n")


def write_png(path, width=320, height=240):
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


# The 2.5 text project of the ProjectFileFormat tests, and XML fragments of it that the drivers replace
TEST_PROJECT = os.path.join(os.path.dirname(HERE), "data", "ProjectFileFormat-2.5.ntp")
BACKGROUND_TAG = b"<Background_project>1</Background_project>"
GUI_TAG = b"<Background_project>0</Background_project>"  # saved by the GUI
DOT_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.Dot</Plugin_id>"
MISSING_PLUGIN_TAG = b"<Plugin_id>fr.inria.built-in.NoSuchPlugin</Plugin_id>"


def replace_first(data, old, new, after=None):
    """Replaces the first occurrence of old, or the first one after after.

    Args:
      data: the project file contents (bytes).
      old: the bytes to replace.
      new: the replacement.
      after: if given, only an occurrence of old after the first occurrence
        of after is replaced.

    Returns:
      The new contents.

    Raises:
      ValueError: old (or after) is not in data.
    """
    start = 0
    if after is not None:
        start = data.find(after)
        if start == -1:
            raise ValueError("%r not found in the test project" % after)
    index = data.find(old, start)
    if index == -1:
        raise ValueError("%r not found in the test project" % old)
    return data[:index] + new + data[index + len(old):]
