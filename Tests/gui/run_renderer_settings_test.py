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

"""Headless test of the cache and thread --setting options of NatronRenderer (#1571).

Usage: run_renderer_settings_test.py /path/to/NatronRenderer [--debugger]

The --setting commands run before the caches are created: setting the size of a cache used to
crash NatronRenderer at startup. NatronRenderer -t
runs a tiny project script with these settings, which checks that the settings have the values
given on the command line and that the caches are in the given disk cache path, and exits with
natron.quit(0). The exit status must be 0.

Exit status: 0 on success, 1 on failure.
--debugger runs NatronRenderer under natron-lldb.sh (see run_gui_smoke_test.py), or set
NATRON_TEST_WRAPPER.
"""

import os
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

# name, value given to --setting (a Python expression), value read back by the script
SETTINGS = [
    ("maxRAMPercent", "10", 10),
    ("maxViewerDiskCache", "1", 1),
    ("maxDiskCacheNode", "2", 2),
    ("noRenderThreads", "2", 2),
    ("nThreadsPerEffect", "2", 2),
    ("useThreadPool", "False", False),
    ("viewerTiling", "7", 7),
]

SCRIPT = """
import os, sys
import NatronEngine
from NatronEngine import natron

status = 1
try:
    settings = natron.getSettings()
    for name, expected in %(expected)r:
        value = settings.getParam(name).getValue()
        assert value == expected, "%%s is %%r, expected %%r" %% (name, value, expected)
    cacheDir = os.environ["NATRON_SETTINGS_TEST_CACHE"]
    assert settings.getParam("diskCachePath").getValue() == cacheDir, settings.getParam("diskCachePath").getValue()
    # the caches were created in the disk cache path given by --setting
    for cache in ("ViewerCache", "DiskCache"):
        assert os.path.isdir(os.path.join(cacheDir, cache)), "no %%s in %%s: %%r" %% (cache, cacheDir, os.listdir(cacheDir))
    # a tiny project
    app = natron.getInstance(0)
    dot = app.createNode("fr.inria.built-in.Dot")
    assert dot is not None
    assert app.waitForRenders(10)
    sys.stdout.write("settings test script passed\\n"); sys.stdout.flush()
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
"""


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    wrapper = []
    if "--debugger" in argv[2:]:
        wrapper = [os.path.join(HERE, "natron-lldb.sh")]
    elif os.environ.get("NATRON_TEST_WRAPPER"):
        wrapper = shlex.split(os.environ["NATRON_TEST_WRAPPER"])
    outDir = tempfile.mkdtemp(prefix="natron-settings-test-")
    try:
        cacheDir = os.path.join(outDir, "cache").replace("\\", "/")
        script = os.path.join(outDir, "settings_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT % {"expected": [(name, expected) for name, _, expected in SETTINGS]})
        env = dict(os.environ)
        # the environment variable takes precedence over the diskCachePath setting (#1265)
        env.pop("NATRON_DISK_CACHE_PATH", None)
        env["NATRON_SETTINGS_TEST_CACHE"] = cacheDir
        # Debug builds: no trapped floating point exception (#1537), as with Natron --test
        env["NATRON_NO_FP_TRAPS"] = "1"
        cmd = wrapper + [argv[1], "--no-settings"]
        # the disk cache path first: the caches are created there
        cmd += ["--setting", "diskCachePath=%r" % cacheDir]
        for name, value, _ in SETTINGS:
            cmd += ["--setting", "%s=%s" % (name, value)]
        cmd += ["-t", script]
        print("Running: " + " ".join(cmd))
        sys.stdout.flush()
        try:
            proc = subprocess.run(cmd, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, timeout=300)
        except subprocess.TimeoutExpired:
            print("FAILED: timeout")
            return 1
        out = proc.stdout.decode(errors="replace")
        sys.stdout.write(out)
        print("NatronRenderer exited with status %d" % proc.returncode)
        ok = True
        if proc.returncode != 0:
            print("FAILED: the exit status should be 0")
            ok = False
        if "settings test script passed" not in out:
            print("FAILED: the test script did not pass")
            ok = False
        if ok:
            print("renderer settings test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(outDir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
