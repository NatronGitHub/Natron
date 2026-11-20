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

# GUI smoke test, run inside Natron by run_gui_smoke_test.py:
#   Natron --no-dialogs gui_smoke_test_script.py
# The environment gives the input image (NATRON_GUI_TEST_IMAGE) and the output
# directory for the screenshots (NATRON_GUI_TEST_OUTDIR).
# The test runs once the event loop has started, and ends with natron.quit(status):
# 0 on success, 1 on failure, 77 if the plug-ins it needs are not available.

import os
import sys
import traceback

import NatronEngine
import NatronGui
from NatronGui import natron

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtGui, QtWidgets

SKIP = 77
READ_ID = "fr.inria.built-in.Read"
TRANSFORM_ID = "net.sf.openfx.TransformPlugin"
VIEWER_ID = "fr.inria.built-in.Viewer"


class TestFailure(Exception):
    pass


# In the GUI, sys.stdout and sys.stderr go to the Script Editor: log to the real stderr
STDERR = sys.__stderr__ or sys.stderr


def log(msg):
    STDERR.write("[gui-smoke-test] %s\n" % msg)
    STDERR.flush()


def check(cond, msg):
    if not cond:
        raise TestFailure(msg)
    log("ok: " + msg)


def imageStats(path):
    """Returns (fraction of pixels that are not almost black, number of distinct colours)
    on a grid of samples of the image file."""
    img = QtGui.QImage(path)
    if img.isNull():
        return (0., 0)
    w, h = img.width(), img.height()
    n = 0
    notBlack = 0
    colours = set()
    step = max(1, min(w, h) // 64)
    for y in range(0, h, step):
        for x in range(0, w, step):
            c = QtGui.QColor(img.pixel(x, y))
            n += 1
            if c.red() + c.green() + c.blue() > 24:
                notBlack += 1
            colours.add((c.red() >> 3, c.green() >> 3, c.blue() >> 3))
    return (float(notBlack) / max(n, 1), len(colours))


def checkScreenshot(app, path, scriptName=""):
    check(app.saveScreenshot(path, scriptName), "saveScreenshot(%r, %r)" % (os.path.basename(path), scriptName))
    check(os.path.isfile(path), "%s exists" % os.path.basename(path))
    notBlack, colours = imageStats(path)
    log("%s: %.0f%% not black, %d colours" % (os.path.basename(path), 100. * notBlack, colours))
    check(notBlack > 0.2 and colours > 8, "%s is not black or uniform" % os.path.basename(path))


def processEventsFor(seconds):
    """Processes the GUI events for a while, so that the renders and redraws happen.
    (The viewer input events, the render wait and the undo functions are in #1455.)"""
    timer = QtCore.QElapsedTimer()
    timer.start()
    while timer.elapsed() < seconds * 1000.:
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)


def runTest():
    app = natron.getGuiInstance(0)
    outDir = os.environ["NATRON_GUI_TEST_OUTDIR"]
    imagePath = os.environ["NATRON_GUI_TEST_IMAGE"]

    ids = natron.getPluginIDs()
    if READ_ID not in ids or TRANSFORM_ID not in ids or not any(i.startswith("fr.inria.openfx.Read") for i in ids):
        log("skipped: the Read and Transform plug-ins are not available (set OFX_PLUGIN_PATH to openfx-io and openfx-misc)")
        return SKIP

    # Read -> Transform -> Viewer1
    read = app.createNode(READ_ID)
    check(read is not None, "created a Read node")
    read.getParam("filename").set(imagePath)
    transform = app.createNode(TRANSFORM_ID)
    check(transform is not None, "created a Transform node")
    transform.connectInput(0, read)
    # node previews on: the preview thread renders while the test runs and quits
    for node in (read, transform):
        p = node.getParam("enablePreview")
        if p is not None:
            p.set(True)
    # Natron only creates a viewer for a new empty project, not when it runs a script
    viewerNode = app.getNode("Viewer1")
    if viewerNode is None:
        viewerNode = app.createNode(VIEWER_ID)
    check(viewerNode is not None and viewerNode.getScriptName() == "Viewer1", "Viewer1 exists")
    viewerNode.connectInput(0, transform)

    viewer = app.getViewer("Viewer1")
    check(viewer is not None, "getViewer('Viewer1')")
    viewer.renderCurrentFrame(True)
    processEventsFor(3.)

    checkScreenshot(app, os.path.join(outDir, "mainWindow.png"))
    checkScreenshot(app, os.path.join(outDir, "Viewer1.png"), "Viewer1")
    check(not app.saveScreenshot(os.path.join(outDir, "none.png"), "noSuchTab"), "saveScreenshot of an unknown tab fails")

    # the other tabs, including the OpenGL ones
    for tab in ("nodeGraph", "properties", "curveEditor", "dopeSheetEditor"):
        path = os.path.join(outDir, tab + ".png")
        if app.saveScreenshot(path, tab):
            log("saved %s" % os.path.basename(path))

    # previews: change the Transform a few times while the previews render
    translate = transform.getParam("translate")
    t0 = translate.get()
    for i in range(5):
        translate.set(t0.x + 10. * i, t0.y)
        processEventsFor(0.5)
    return 0


def main():
    status = 1
    try:
        status = runTest()
    except TestFailure as e:
        log("FAILED: %s" % e)
    except Exception:
        traceback.print_exc(file=STDERR)
    # Also write the status to a file: with some OFX plug-ins, Natron may crash at exit,
    # after natron.quit() (see https://github.com/NatronGitHub/Natron/issues/1253)
    with open(os.path.join(os.environ["NATRON_GUI_TEST_OUTDIR"], "status.txt"), "w") as f:
        f.write("%d\n" % status)
    log("natron.quit(%d)" % status)
    natron.quit(status)


# The script is run while the project is created: run the test once the event loop
# has started and the main window is shown.
QtCore.QTimer.singleShot(500, main)
