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

"""The graphs of the render soak test (run_render_soak_test.py), built and rendered inside NatronRenderer.

NatronRenderer runs this script, as a Python project (the Write nodes are then rendered from the
command line) or with -t. The environment gives the parameters:
- SOAK_OUT: the directory of the rendered files;
- SOAK_GRAPH: "writers" (generators, Reformat, Grade and Transform driven by Python expressions, Transform
  with motion blur, Merge, FrameHold, Blur, DiskCache, then four Write nodes: PNG, JPEG, EXR, TIFF), or
  "sequential" (the test plug-in net.natron.test.Sequential, which Natron renders in frame order on one
  thread, before a PNG Write, and a WriteFFmpeg, which needs its frames in order);
- SOAK_SIZE: "WxH", the size of the rendered images;
- SOAK_INPUT: the directory of the image sequence in_####.png that the Read node of the writers graph reads;
- SOAK_SAVE: if set, save the project there and quit (no render);
- SOAK_RENDER: if set, render from Python (app.render) instead of from the command line:
  "first-last", and SOAK_WRITERS (comma-separated, default all), SOAK_ABORT_FRAME (natron.quit(3) from the
  afterFrameRender callback of the last Write at that frame, 0 for no abort), SOAK_HANG_FRAME (the first Write
  hangs at that frame: checks the watchdog).

It prints "SOAK writer <name> <file pattern>" for each Write node it created, and "SOAK SKIP: <reason>" (exit
status 77) when a plug-in it needs is missing.
"""

import os
import sys
import threading

from NatronEngine import natron

ABORT_STATUS = 3
SKIP = 77
# chunk sizes of the renders from Python: many one- and two-frame renders, whose end the scheduler
# must notice (#1607), and longer ones
CHUNKS = [1, 2, 3, 50, 1, 7, 200, 2]

IDS = set(natron.getPluginIDs())
# natron.quit() is recent (2.6): without it (to soak an older Natron), no abort, and the exit status is 0
CAN_QUIT = hasattr(natron, "quit")


def quit_with(status):
    """Exits NatronRenderer with status, if this Natron has natron.quit()."""
    if CAN_QUIT:
        natron.quit(status)


def say(message):
    sys.stdout.write("SOAK " + message + "\n")
    sys.stdout.flush()


def make(app, plugin, name, inputs=(), **params):
    """Creates a node of plugin (None if it is missing), connects its inputs and sets its parameters.

    A parameter that the plug-in doesn't have only prints a warning: the graph doesn't depend on one
    version of the plug-ins. A value that is a tuple sets every dimension; ("expr", e) sets the Python
    expression e on every dimension.
    """
    if plugin not in IDS:
        say("warning: no plug-in %s, node %s not created" % (plugin, name))
        return None
    node = app.createNode(plugin)
    node.setScriptName(name)
    for i, input_node in enumerate(inputs):
        if input_node is not None:
            node.connectInput(i, input_node)
    for key, value in params.items():
        param = node.getParam(key)
        if param is None:
            say("warning: %s has no parameter %s" % (name, key))
            continue
        if isinstance(value, tuple) and value and value[0] == "expr":
            for dim in range(param.getNumDimensions()):
                param.setExpression(value[1], False, dim)
        elif isinstance(value, tuple):
            param.set(*value)
        else:
            param.set(value)
    return node


VIDEO_WRITERS = set()  # the names of the Write nodes that write a video (one file)


def make_write(app, out_dir, name, source, pattern, writers):
    """Creates a Write node to out_dir/pattern after source, and appends it to writers (if Write exists)."""
    node = make(app, "fr.inria.built-in.Write", name, [source])
    if node is None:
        return
    node.getParam("filename").set(os.path.join(out_dir, pattern))
    say("writer %s %s" % (name, pattern))
    if "#" not in pattern:
        VIDEO_WRITERS.add(name)
    writers.append(node)


def reformat(app, name, source, width, height):
    """A Reformat of source to width x height (rodlie's repro of #248 stalled at a Reformat node), or source."""
    return make(app, "net.sf.openfx.Reformat", name, [source], reformatType=1, boxSize=(width, height),
                boxFixed=True) or source


def build_writers(app, out_dir, width, height):
    """Four Write nodes after a graph like those of the reports in #248."""
    writers = []
    clock = make(app, "net.sf.openfx.NoOpPlugin", "Clock")
    if clock is None or "net.sf.openfx.CheckerBoardPlugin" not in IDS:
        return writers
    # every expression reads this one (odditica's "clock" NoOp node in #248)
    clock.createDoubleParam("t", "t").setExpression("(frame % 48) / 48.", False, 0)
    checker = reformat(app, "CheckerReformat", make(app, "net.sf.openfx.CheckerBoardPlugin", "Checker"),
                       width, height)
    noise = make(app, "net.sf.openfx.Noise", "Noise", seed=("expr", "frame"))
    if noise is not None:
        noise = reformat(app, "NoiseReformat", noise, width, height)
    grade = make(app, "net.sf.openfx.GradePlugin", "Grade", [checker],
                 multiply=("expr", "0.6 + 0.4 * Clock.t.get()")) or checker
    transform = make(app, "net.sf.openfx.TransformPlugin", "Transform", [grade],
                     rotate=("expr", "360. * Clock.t.get()"), motionBlur=0.5) or grade
    hold = make(app, "net.sf.openfx.FrameHold", "Hold", [noise], firstFrame=1) if noise is not None else None
    merge = make(app, "net.sf.openfx.MergePlugin", "Merge", [hold or noise, transform]) or transform
    input_dir = os.environ.get("SOAK_INPUT")
    if input_dir:
        # most reports of #248 read image sequences: a PNG sequence, shorter than the render, that loops
        read = make(app, "fr.inria.built-in.Read", "Read")
        if read is not None:
            read.getParam("filename").set(os.path.join(input_dir, "in_####.png"))
            for key in ("before", "after"):
                param = read.getParam(key)
                if param is not None:
                    try:
                        param.set("loop")
                    except Exception:  # pylint: disable=broad-except
                        param.set(1)  # Natron without choice options by name: "Loop" is the second option
            merge = make(app, "net.sf.openfx.MergePlugin", "MergeRead",
                         [merge, reformat(app, "ReadReformat", read, width, height)]) or merge
    blur = make(app, "net.sf.cimg.CImgBlur", "Blur", [merge], size=(3., 3.)) or merge
    cached = make(app, "fr.inria.built-in.DiskCache", "DiskCache", [blur]) or blur
    color = make(app, "net.sf.openfx.ColorCorrectPlugin", "ColorCorrect", [merge],
                 MasterSaturation=("expr", "1. - 0.5 * Clock.t.get()")) or merge
    make_write(app, out_dir, "WritePNG", cached, "png_####.png", writers)
    make_write(app, out_dir, "WriteJPEG", color, "jpg_####.jpg", writers)
    make_write(app, out_dir, "WriteEXR", transform, "exr_####.exr", writers)
    make_write(app, out_dir, "WriteTIFF", blur, "tif_####.tif", writers)
    return writers


def build_sequential(app, out_dir, width, height):
    """A sequential plug-in before a PNG Write (#1413), and WriteFFmpeg, a writer that needs its frames in order."""
    writers = []
    if "net.sf.openfx.CheckerBoardPlugin" not in IDS:
        return writers
    checker = reformat(app, "CheckerReformat", make(app, "net.sf.openfx.CheckerBoardPlugin", "Checker"),
                       width, height)
    sequential = make(app, "net.natron.test.Sequential", "Sequential", [checker])
    if sequential is not None:
        grade = make(app, "net.sf.openfx.GradePlugin", "Grade", [sequential], multiply=("expr", "1. + (frame % 7) / 7."))
        make_write(app, out_dir, "WriteSequential", grade or sequential, "seq_####.png", writers)
    if "fr.inria.openfx.WriteFFmpeg" in IDS:
        transform = make(app, "net.sf.openfx.TransformPlugin", "Transform", [checker],
                         translate=("expr", "frame % 32")) or checker
        make_write(app, out_dir, "WriteFFmpeg", transform, "video.mov", writers)
    return writers


def render_from_python(app, writers, first, last, abort_frame):
    """Renders the writers with app.render(): the first one in short chunks (one render each, then its first 100
    frames again one by one), the middle ones together, the last one alone, aborted with natron.quit() at
    abort_frame if it is not 0.

    Returns the exit status: ABORT_STATUS if the render was aborted, else 0.
    """
    chunked = writers[0]
    if chunked.getScriptName() in VIDEO_WRITERS:
        # each render writes the whole video file again: render it at once
        app.render(chunked, first, last)
        writers = writers[1:]
        if not writers:
            return 0
        chunked = writers[0]
    hang_frame = int(os.environ.get("SOAK_HANG_FRAME", "0"))
    if hang_frame:
        # a render that hangs at that frame, to check the watchdog of run_render_soak_test.py
        def hang(frame, thisNode, app):  # pylint: disable=invalid-name,unused-argument
            if frame == hang_frame:
                say("hanging at frame %d (SOAK_HANG_FRAME)" % frame)
                threading.Event().wait()
        import __main__  # pylint: disable=import-outside-toplevel
        __main__.soakHang = hang
        chunked.getParam("afterFrameRender").set("soakHang")
    start, i = first, 0
    while start <= last:
        end = min(last, start + CHUNKS[i % len(CHUNKS)] - 1)
        app.render(chunked, start, end)
        start, i = end + 1, i + 1
    # the same frames again, one render each: the images upstream are cached, so a render can end before the
    # scheduler waits for its end
    for frame in range(first, min(last, first + 99) + 1):
        app.render(chunked, frame, frame)
    say("rendered %s in %d renders" % (chunked.getScriptName(), i + min(last, first + 99) - first + 1))
    together = writers[1:-1]
    if together:
        app.render([(w, first, last) for w in together])
        say("rendered %s together" % ",".join(w.getScriptName() for w in together))
    if len(writers) > 1:
        alone = writers[-1]
        aborted = []
        if abort_frame and not CAN_QUIT:
            say("no natron.quit(): no abort")
        elif abort_frame:
            def after_frame(frame, thisNode, app):  # pylint: disable=invalid-name,unused-argument
                if frame >= abort_frame and not aborted:
                    say("aborting at frame %d" % frame)
                    aborted.append(frame)
                    natron.quit(ABORT_STATUS)
            import __main__  # pylint: disable=import-outside-toplevel
            __main__.soakAfterFrame = after_frame
            alone.getParam("afterFrameRender").set("soakAfterFrame")
        app.render(alone, first, last)
        if aborted:
            # natron.quit() aborted the render, and app.render() returned: the script goes on
            say("aborted %s" % alone.getScriptName())
            return ABORT_STATUS
        say("rendered %s" % alone.getScriptName())
    return 0


def main():
    app = natron.getInstance(0)
    out_dir = os.environ["SOAK_OUT"]
    width, height = (int(v) for v in os.environ.get("SOAK_SIZE", "320x180").split("x"))
    graph = os.environ.get("SOAK_GRAPH", "writers")
    build = build_sequential if graph == "sequential" else build_writers
    writers = build(app, out_dir, width, height)
    if not writers:
        say("SKIP: no Write node could be created for the %s graph (OFX_PLUGIN_PATH=%s)"
            % (graph, os.environ.get("OFX_PLUGIN_PATH", "")))
        quit_with(SKIP)
        return
    if os.environ.get("SOAK_SAVE"):
        app.saveProjectAs(os.environ["SOAK_SAVE"])
        say("saved " + os.environ["SOAK_SAVE"])
        quit_with(0)
        return
    frames = os.environ.get("SOAK_RENDER")
    if frames:
        wanted = [w for w in os.environ.get("SOAK_WRITERS", "").split(",") if w]
        if wanted:
            writers = [w for w in writers if w.getScriptName() in wanted]
        first, last = (int(v) for v in frames.split("-"))
        status = render_from_python(app, writers, first, last, int(os.environ.get("SOAK_ABORT_FRAME", "0")))
        say("done")
        quit_with(status)


# NatronRenderer runs this file as a script (with -t, or as a Python project), never imports it
main()
