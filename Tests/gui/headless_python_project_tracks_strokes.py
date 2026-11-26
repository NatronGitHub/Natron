"""The Python project format keeps the tracks of a Tracker and the paint strokes of a RotoPaint.

A project (a Python project written by hand, with the Python API that the format uses) has a Tracker with
tracks (animated centers, user keyframes, labels; a pattern-matching one if openfx-misc's TrackerPM is
installed) and a RotoPaint with paint strokes (one with two lines, one in a sub-layer) and a Bezier between
them. It is saved as text (a.ntp), which is loaded and saved as Python (b.py), which is loaded and saved as
text (c.ntp). The tracks and the items of the RotoPaint, dumped with the Python API after each load (points,
pressures, timestamps, centers per frame...), must be the same, exactly, and so must the TrackerContext and
RotoContext elements of a.ntp and c.ntp. The values need 17 digits (e.g. 0.1 + 0.2).

Skipped without openfx-misc's Roto, Merge, Constant, Transform, TimeOffset and FrameHold plug-ins.
"""

import difflib
import os
import re
import tempfile

import NatronEngine

import natronheadless

PLUGINS = ("net.sf.openfx.RotoPlugin", "net.sf.openfx.MergePlugin", "net.sf.openfx.ConstantPlugin",
           "net.sf.openfx.TransformPlugin", "net.sf.openfx.timeOffset", "net.sf.openfx.FrameHold")
TRACKER_PM = "net.sf.openfx.TrackerPM"
FORMATS = NatronEngine.Natron.ProjectFileFormatEnum
# Parameters of the paint strokes and shapes that the dump has (Engine/RotoContextPrivate.h)
ITEM_PARAMS = ("brushSize", "brushHardness", "opacity", "color", "center", "lifeTimeFrame")
FRAMES = range(1, 6)

SOURCE = """# -*- coding: utf-8 -*-
# Natron Project

import NatronEngine

STROKE = NatronEngine.Natron.RotoStrokeType


def createProject(app):
    group = app
    trackerNode = app.createNode("fr.inria.built-in.Tracker", 1, group)
    trackerNode.setScriptName("Tracker1")
    tracker = trackerNode.getTrackerContext()
    track = tracker.createTrack(False)
    track.setScriptName("corner")
    track.setLabel("Corner A")
    center = track.getParam("centerPoint")
    for frame in range(1, 6):
        center.setValueAtTime(10.25 * frame + 0.1 + 0.2, frame, 0)
        center.setValueAtTime(20 + frame / 3.0, frame, 1)
    track.setUserKeyframe(1)
    track.setUserKeyframe(4)
    if %(pm)s:
        track = tracker.createTrack(True)
        track.setScriptName("pattern")
        track.getParam("centerPoint").setValue(0.1 + 0.2, 0)

    paint = app.createNode("fr.inria.built-in.RotoPaint", 1, group)
    paint.setScriptName("RotoPaint1")
    roto = paint.getRotoContext()
    base = roto.getBaseLayer()
    stroke = roto.createStroke(STROKE.eRotoStrokeTypeSolid)
    stroke.setScriptName("Brush1")
    stroke.setLabel("Brush one")
    base.addItem(stroke)
    stroke.addStroke([(100 + 0.1 + 0.2, 100, 1, 0), (150, 120 + 1 / 3.0, 0.75, 0.04), (200, 160, 0.5, 0.09)])
    stroke.addStroke([(300, 300, 0.25, 0), (320.7, 310, 0.5, 0.02)])
    stroke.getParam("brushSize").setValue(12.5)
    stroke.getParam("color").setValue(0.1 + 0.2, 0)
    bezier = roto.createBezier(10, 10, 1)
    bezier.setScriptName("Shape")
    base.addItem(bezier)
    bezier.addControlPoint(50, 10)
    bezier.addControlPoint(30, 40)
    bezier.setCurveFinished(True)
    sub = roto.createLayer()
    sub.setScriptName("Sub")
    base.addItem(sub)
    clone = roto.createStroke(STROKE.eRotoStrokeTypeClone)
    clone.setScriptName("Clone1")
    sub.addItem(clone)
    clone.addStroke([(10, 20, 0.5, 0), (30, 40, 0.6, 0.01), (50, 45, 0.7, 0.03), (70, 47, 0.8, 0.05)])
    clone.getParam("lifeTimeFrame").setValue(3)
    clone.setLocked(True)


if __name__ == "__main__":
    createProject(app)
"""


def dump_param(param):
    """Returns the values and keyframes of param, for each dimension."""
    dims = []
    for dim in range(param.getNumDimensions()):
        entry = {}
        try:
            entry["value"] = param.getValue(dim)
        except TypeError:
            entry["value"] = param.getValue()
        if hasattr(param, "getNumKeys"):
            keys = []
            for i in range(param.getNumKeys(dim)):
                _, time = param.getKeyTime(i, dim)
                keys.append((time, param.getValueAtTime(time, dim)))
            entry["keys"] = keys
        dims.append(entry)
    return dims


def dump_tracks(node):
    """Returns the tracks of the Tracker node: names, user keyframes, parameters, centers per frame."""
    tracks = []
    for track in node.getTrackerContext().getAllTracks():
        center = track.getParam("centerPoint")
        tracks.append({
            "name": track.getScriptName(),
            "label": track.getLabel(),
            "userKeyframes": track.getUserKeyframes(),
            "params": {p.getScriptName(): dump_param(p) for p in track.getParams()},
            "centers": [(center.getValueAtTime(f, 0), center.getValueAtTime(f, 1)) for f in FRAMES],
        })
    return tracks


def dump_items(roto, layer):
    """Returns the items of layer (strokes, shapes and layers, recursively), in their order."""
    items = []
    for child in layer.getChildren():
        # the children are ItemBase objects: getItemByName() gives a StrokeItem, a BezierCurve or a Layer
        item = roto.getItemByName(child.getScriptName())
        entry = {
            "type": type(item).__name__,
            "name": item.getScriptName(),
            "label": item.getLabel(),
            "visible": item.getVisible(),
            "locked": item.getLocked(),
        }
        if isinstance(item, NatronEngine.Layer):
            entry["items"] = dump_items(roto, item)
        else:
            entry["params"] = {name: dump_param(item.getParam(name)) for name in ITEM_PARAMS
                               if item.getParam(name) is not None}
        if isinstance(item, NatronEngine.StrokeItem):
            entry["brush"] = str(item.getBrushType())
            entry["lines"] = [item.getPoints(i) for i in range(item.getNumStrokes())]
        elif isinstance(item, NatronEngine.BezierCurve):
            keys = item.getKeyframes()
            entry["keys"] = keys
            entry["points"] = [[(item.getControlPointPosition(i, t), item.getFeatherPointPosition(i, t))
                                for i in range(item.getNumControlPoints())] for t in keys]
        items.append(entry)
    return items


def dump(app):
    """Returns the state of the Tracker and the RotoPaint of app."""
    tracker = app.getNode("Tracker1")
    paint = app.getNode("RotoPaint1")
    natronheadless.check(tracker is not None and paint is not None, "no Tracker1 or RotoPaint1 node")
    roto = paint.getRotoContext()
    return {"tracks": dump_tracks(tracker), "roto": dump_items(roto, roto.getBaseLayer())}


def contexts(path):
    """Returns the TrackerContext and RotoContext elements of the text project path.

    Without the attributes of the archive (class and object ids), which depend on what else it has.
    """
    with open(path, encoding="utf-8") as f:
        text = f.read()
    found = []
    for name in ("TrackerContext", "RotoContext"):
        elements = re.findall(r"<%s[ >].*?</%s>" % (name, name), text, re.DOTALL)
        natronheadless.check(len(elements) == 1, "%s: %d %s elements" % (path, len(elements), name))
        found.append(re.sub(r' (class_id|class_id_reference|object_id|object_id_reference|tracking_level)="[^"]*"',
                            "", elements[0]))
    return "\n".join(found)


def load(app, path):
    """Loads the project path in app (NatronRenderer has a single App) and returns the App."""
    loaded = app.loadProject(path)
    natronheadless.check(loaded is not None, "could not load " + path)
    return loaded


def main(app):
    """Saves the project as text, Python, then text, and compares."""
    natronheadless.require_plugins(*PLUGINS)
    pm = TRACKER_PM in NatronEngine.natron.getPluginIDs()
    if not pm:
        print("no %s: the pattern-matching track is not tested" % TRACKER_PM)
    with tempfile.TemporaryDirectory() as tmp:
        source = os.path.join(tmp, "source.py")
        with open(source, "w", encoding="utf-8") as f:
            f.write(SOURCE % {"pm": pm})
        text1, python, text2 = (os.path.join(tmp, name) for name in ("a.ntp", "b.py", "c.ntp"))
        natronheadless.check(load(app, source).saveProjectCopy(text1, FORMATS.eProjectFileFormatText), "text save failed")
        loaded = load(app, text1)
        before = dump(loaded)
        natronheadless.check(len(before["tracks"]) == (2 if pm else 1), "tracks: %r" % before["tracks"])
        natronheadless.check([i["name"] for i in before["roto"]] == ["Brush1", "Shape", "Sub"], "items: %r" % before["roto"])
        natronheadless.check(len(before["roto"][0]["lines"]) == 2, "Brush1: %r" % before["roto"][0])
        natronheadless.check(before["roto"][2]["items"][0]["lines"][0][1] == (30, 40, 0.6, 0.01), "Clone1: %r" % before["roto"][2])
        natronheadless.check(loaded.saveProjectCopy(python, FORMATS.eProjectFileFormatPython), "Python save failed")

        loaded = load(app, python)
        after = dump(loaded)
        natronheadless.check(loaded.saveProjectCopy(text2, FORMATS.eProjectFileFormatText), "second text save failed")
        if after != before:
            for key in ("tracks", "roto"):
                if after[key] != before[key]:
                    print("%s before the Python save: %r" % (key, before[key]))
                    print("%s after: %r" % (key, after[key]))
            raise AssertionError("the tracks or the paint strokes changed in the Python project " + python)

        expected, got = contexts(text1), contexts(text2)
        if got != expected:
            print("".join(difflib.unified_diff(expected.splitlines(True), got.splitlines(True), text1, text2)))
            raise AssertionError("the TrackerContext or RotoContext of %s and %s differ" % (text1, text2))


natronheadless.run(main)
