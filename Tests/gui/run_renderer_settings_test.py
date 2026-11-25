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

Exit status: 0 on success, 1 on failure. --debugger runs NatronRenderer under natron-lldb.sh, or set
NATRON_TEST_WRAPPER (see natronrunner.py).
"""

import os
import shutil
import sys
import tempfile

import natronrunner

TIMEOUT = 300  # seconds

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
    cache_dir = os.environ["NATRON_SETTINGS_TEST_CACHE"]
    assert settings.getParam("diskCachePath").getValue() == cache_dir, settings.getParam("diskCachePath").getValue()
    # the caches were created in the disk cache path given by --setting
    for cache in ("ViewerCache", "DiskCache"):
        assert os.path.isdir(os.path.join(cache_dir, cache)), "no %%s in %%s: %%r" %% (cache, cache_dir, os.listdir(cache_dir))
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
    cmd_wrapper = natronrunner.wrapper(argv[2:])
    out_dir = tempfile.mkdtemp(prefix="natron-settings-test-")
    try:
        cache_dir = os.path.join(out_dir, "cache").replace("\\", "/")
        script = os.path.join(out_dir, "settings_test.py")
        with open(script, "w") as f:
            f.write(SCRIPT % {"expected": [(name, expected) for name, _, expected in SETTINGS]})
        env = natronrunner.natron_env(out_dir, XDG_CACHE_HOME=os.path.join(out_dir, "xdg-cache"),
                         NATRON_SETTINGS_TEST_CACHE=cache_dir)
        # the environment variable takes precedence over the diskCachePath setting (#1265): unset, so that the
        # test checks the setting. Before the setting applies, the first refreshDiskCacheLocation() uses the
        # default location: on Linux, a private one (XDG_CACHE_HOME), not the user's (#1569). macOS and Windows
        # have no environment override for it, but nothing is written there except a short-lived instance lock.
        del env["NATRON_DISK_CACHE_PATH"]
        cmd = cmd_wrapper + [argv[1], "--no-settings"]
        # the disk cache path first: the caches are created there
        cmd += ["--setting", "diskCachePath=%r" % cache_dir]
        for name, value, _ in SETTINGS:
            cmd += ["--setting", "%s=%s" % (name, value)]
        cmd += ["-t", script]
        status, out = natronrunner.run(cmd, env, TIMEOUT)
        sys.stdout.write(out)
        if status is None:
            print("FAILED: timeout")
            return 1
        print("NatronRenderer exited with status %d" % status)
        ok = True
        if status != 0:
            print("FAILED: the exit status should be 0")
            ok = False
        if "settings test script passed" not in out:
            print("FAILED: the test script did not pass")
            ok = False
        if ok:
            print("renderer settings test passed")
        return 0 if ok else 1
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
