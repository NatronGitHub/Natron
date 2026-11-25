"""The shapes of Roto and RotoPaint nodes look the same with every version of openfx-misc's Roto plug-in.

Each shape is drawn by an internal node of that plug-in (RotoDrawableItem::createNodes()): RotoPaint shapes keep
their color, Roto shapes are drawn into alpha, and a shape that extends beyond the project format is not clipped
at its edge, whatever the defaults of the plug-in.

Skipped when openfx-misc's Roto, Merge and Transform, or openfx-io's WriteOIIO, are not installed, or without OCIO.
"""
import os
import tempfile

from natronheadless import check, check_rendered, read_pfm_pixels, require_plugins, run

PLUGINS = ("net.sf.openfx.RotoPlugin", "net.sf.openfx.MergePlugin", "net.sf.openfx.TransformPlugin",
           "fr.inria.openfx.WriteOIIO")
COLOR = (0.25, 0.5, 0.75)
FRAME = 1
# The rectangle covers [-100, 100) x [100, 300): its left half is outside the project format (1920x1080).
# The Transform moves it to [100, 300) x [100, 300), into the rendered format.
SHIFT = 200.
INSIDE_FORMAT = (250, 200)  # was at x = 50
OUTSIDE_FORMAT = (150, 200)  # was at x = -50
EMPTY = (600, 600)


def render(app, source, path, components, points):
    """Renders source in the project format, as components ("RGB" or "Alpha"), and returns the pixels at points."""
    writer = app.createWriter(path)
    check(writer is not None, "createWriter(%s) failed" % path)
    writer.connectInput(0, source)
    writer.getParam("outputComponents").set(components)
    writer.getParam("formatType").set("project")  # the pixel coordinates are the canonical coordinates
    app.render(writer, FRAME, FRAME)
    check_rendered(path)
    return read_pfm_pixels(path, points)


def make_shape(app, plugin_id):
    """Creates a node of plugin_id with a rectangle beyond the project format, moved into it by a Transform."""
    node = app.createNode(plugin_id)
    check(node is not None, "createNode(%s) failed" % plugin_id)
    rect = node.getRotoContext().createRectangle(-100., 300., 200., FRAME)  # top-left corner and size
    # keyframes: set() without a frame does not change the parameters of a shape
    rect.getFeatherDistanceParam().set(0., FRAME)
    rect.getColorParam().set(COLOR[0], COLOR[1], COLOR[2], 1., FRAME)
    transform = app.createNode("net.sf.openfx.TransformPlugin")
    check(transform is not None, "createNode(Transform) failed")
    transform.getParam("translate").set(SHIFT, 0.)
    transform.connectInput(0, node)
    return transform


def check_shape(app, plugin_id, outdir, components):
    """Checks the color (RGB) or alpha (Alpha) of a shape, inside and beyond the format, and outside the shape."""
    shape = make_shape(app, plugin_id)
    for comps in components:
        path = os.path.join(outdir, "%s_%s.pfm" % (plugin_id.split(".")[-1], comps))
        drawn = COLOR if comps == "RGB" else (1.,)
        points = ((INSIDE_FORMAT, drawn, ""), (OUTSIDE_FORMAT, drawn, " (beyond the format)"),
                  (EMPTY, (0.,) * len(drawn), " (outside the shape)"))
        pixels = render(app, shape, path, comps, [p[0] for p in points])
        for (xy, want, where), got in zip(points, pixels):
            check(len(got) == len(want) and all(abs(g - w) < 1e-5 for g, w in zip(got, want)),
                  "%s %s at %r%s is %r, expected %r" % (plugin_id, comps, xy, where, got, want))


def main(app):
    """Checks a RotoPaint shape (color and alpha) and a Roto shape (alpha)."""
    require_plugins(*PLUGINS)
    with tempfile.TemporaryDirectory() as outdir:
        # RotoPaint draws R, G, B and A by default: the shape color, premultiplied by its alpha
        check_shape(app, "fr.inria.built-in.RotoPaint", outdir, ("RGB", "Alpha"))
        # Roto draws only A by default
        check_shape(app, "fr.inria.built-in.Roto", outdir, ("Alpha",))


run(main)
