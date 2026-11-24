# Regression test for #1463: when a project is opened in a new window (the current project is not empty),
# onProjectCreated must not run on the new window before the project is loaded; onProjectLoaded runs once
# the project is loaded, as when the project is opened in the current window.
import os
import shutil
import tempfile

import __main__

from natrontest import *

first = app

events = []


def onCreated(app):
    events.append(("created", app.getNode("Marker") is not None))


def onLoaded(app):
    events.append(("loaded", app.getNode("Marker") is not None))


# the callbacks are run as "onCreated(app2)" in __main__
__main__.onCreated = onCreated
__main__.onLoaded = onLoaded

tmpdir = tempfile.mkdtemp(prefix="natron-test-1463-")
try:
    marker = first.createNode("fr.inria.built-in.Dot")
    check(marker is not None, "createNode(Dot) failed")
    check(marker.setScriptName("Marker"), "setScriptName(Marker) failed")
    project = os.path.join(tmpdir, "marked.ntp")
    check(first.saveProjectAs(project), "saveProjectAs(%s) failed" % project)

    # the current project has nodes and was saved: the project to open goes to a new window
    current = os.path.join(tmpdir, "current.ntp")
    check(first.saveProjectAs(current), "saveProjectAs(%s) failed" % current)
    natron.setOnProjectCreatedCallback("onCreated")
    natron.setOnProjectLoadedCallback("onLoaded")
    instances = natron.getNumInstances()
    second = first.loadProject(project)
    wait_idle()
    check(second is not None, "loadProject(%s) failed" % project)
    check(natron.getNumInstances() == instances + 1, "the project was not opened in a new window")
    check(second.getNode("Marker") is not None, "the new window has no Marker node")
    check(events == [("loaded", True)], "callbacks on the new window: %r, expected [('loaded', True)]" % (events,))
finally:
    natron.setOnProjectCreatedCallback("")
    natron.setOnProjectLoadedCallback("")
    shutil.rmtree(tmpdir, ignore_errors=True)
