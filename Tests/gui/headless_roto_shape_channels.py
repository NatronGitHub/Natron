"""The shapes of Roto and RotoPaint nodes look the same with every version of openfx-misc's Roto plug-in.

Each shape is drawn by an internal node of that plug-in (RotoDrawableItem::createNodes()): RotoPaint shapes keep
their color, Roto shapes are drawn into alpha, and a shape that extends beyond the project format is not clipped
at its edge, whatever the defaults of the plug-in.

Run by NatronRenderer -t (see Tests/CMakeLists.txt). Exits with 0 on success, 1 on failure, 77 (skipped)
when openfx-misc's Roto, Merge and Transform, or openfx-io's WriteOIIO, are not installed, or without OCIO.
"""
import os
import struct
import tempfile
import traceback

from NatronEngine import natron

SKIP = 77
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


class Skipped(Exception):
    """The test cannot run here."""


def check(condition, message=""):
    """Like assert, but not removed by python -O."""
    if not condition:
        raise AssertionError(message)


def read_pfm_pixels(path, points):
    """Returns the pixels at points ((x, y), y = 0 at the bottom) of a PFM image, as tuples of floats."""
    with open(path, "rb") as f:
        magic = f.readline().strip()
        check(magic in (b"PF", b"Pf"), "%s is not a PFM file" % path)
        channels = 3 if magic == b"PF" else 1
        width, height = (int(v) for v in f.readline().split())
        endian = "<" if float(f.readline()) < 0 else ">"
        start = f.tell()
        pixels = []
        for x, y in points:
            check(0 <= x < width and 0 <= y < height, "%r is outside the %dx%d image %s" % ((x, y), width, height, path))
            f.seek(start + (y * width + x) * channels * 4)
            pixels.append(struct.unpack("%s%df" % (endian, channels), f.read(channels * 4)))
        return pixels


def render(app, source, path, components, points):
    """Renders source in the project format, as components ("RGB" or "Alpha"), and returns the pixels at points."""
    writer = app.createWriter(path)
    check(writer is not None, "createWriter(%s) failed" % path)
    writer.connectInput(0, source)
    writer.getParam("outputComponents").set(components)
    writer.getParam("formatType").set("project")  # the pixel coordinates are the canonical coordinates
    app.render(writer, FRAME, FRAME)
    if not os.path.isfile(path) and not os.path.isfile(os.environ.get("OCIO", "")):
        # the writers of openfx-io fail without an OpenColorIO configuration
        raise Skipped("nothing was rendered and there is no OpenColorIO configuration: set OCIO")
    check(os.path.isfile(path), "nothing was rendered to " + path)
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


def main(outdir):
    """Checks a RotoPaint shape (color and alpha) and a Roto shape (alpha)."""
    app = natron.getInstance(0)
    # RotoPaint draws R, G, B and A by default: the shape color, premultiplied by its alpha
    check_shape(app, "fr.inria.built-in.RotoPaint", outdir, ("RGB", "Alpha"))
    # Roto draws only A by default
    check_shape(app, "fr.inria.built-in.Roto", outdir, ("Alpha",))


status = 1
try:
    ids = natron.getPluginIDs()
    missing = [p for p in PLUGINS if p not in ids]
    if missing:
        print("missing plug-ins %s: skipped" % ", ".join(missing))
        status = SKIP
    else:
        with tempfile.TemporaryDirectory() as tmp:
            main(tmp)
        status = 0
except Skipped as e:
    print("skipped: %s" % e)
    status = SKIP
except Exception:  # pylint: disable=broad-except
    traceback.print_exc()
natron.quit(status)
