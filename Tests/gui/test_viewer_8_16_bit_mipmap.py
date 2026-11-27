"""8- and 16-bit images display like the float image when the viewer is zoomed out (#1549).

A float ramp, converted to 8 bits and to 16 bits by the test plug-ins net.natron.test.DepthByte and
net.natron.test.DepthShort, is shown at 62%, 40%, 20% and 12% zoom: mipmap levels 0 (a control, as
in #1549, where the stripes were seen at 62% and 27%), 1, 2 and 3. The viewer uses the mipmap level
of the power of 2 at or above the zoom. The DepthByte and DepthShort plug-ins don't support render
scale, so Natron renders them at full scale and downscales the 8/16-bit images itself. At every
sampled point, across the whole width of the image, the colour displayed for the 8- and 16-bit
images must be the one displayed for the float ramp (within 2 codes): no stripes, no darkening.
"""
import NatronGuiTest
from natrontest import *

RAMP = "net.natron.test.Ramp"  # float RGBA, 512x256, R = G = B = x / 512, A = 1
DEPTHS = (("8-bit", "net.natron.test.DepthByte"), ("16-bit", "net.natron.test.DepthShort"))
# (zoom, mipmap level): 2^-level >= zoom > 2^-(level + 1) (ViewerInstance::getMipmapLevelFromZoomFactor());
# not exactly a power of 2, which a zoom read back slightly above it would move to the next level
ZOOMS = ((0.62, 0), (0.4, 1), (0.2, 2), (0.12, 3))
WIDTH, HEIGHT = 512, 256


def sample_xs(level):
    """16 points from the left edge to the right edge, each at the centre of a pixel of the mipmap level."""
    return [32 * i + 16 + 2 ** (level - 1) for i in range(WIDTH // 32)]


TOLERANCE = 2  # 8-bit display codes

for plugin in (RAMP,) + tuple(p for _, p in DEPTHS):
    require_plugin(plugin)

viewer_node_ = viewer_node(app)
viewer = app.getViewer("Viewer1")
ramp = app.createNode(RAMP)
converters = []
for label, plugin in DEPTHS:
    node = app.createNode(plugin)
    node.connectInput(0, ramp)
    converters.append((label, node))


def displayed_row(node, zoom, xs):
    """The colours displayed at xs along the middle row of the image, for node at zoom."""
    viewer_node_.connectInput(0, node)
    viewer.setZoom(zoom)
    viewer.setCenter(WIDTH / 2.0, HEIGHT / 2.0)
    viewer.renderCurrentFrame(True)
    wait_idle()
    check_close(viewer.getZoom(), zoom, 1e-6, "zoom")
    return [NatronGuiTest.getDisplayedPixel("Viewer1", x, HEIGHT / 2.0) for x in xs]


failures = []
for zoom, level in ZOOMS:
    xs = sample_xs(level)
    reference = displayed_row(ramp, zoom, xs)
    print("zoom %g, float ramp: %r" % (zoom, [c[0] for c in reference]))
    # the float ramp itself is displayed as expected (the 8/16-bit images are compared with it)
    for x, c in zip(xs, reference):
        if x >= 128:  # in the dark part, a mipmap pixel covers too many codes
            check_close(c[0], srgb8(x / float(WIDTH)), 6, "float ramp displayed at x=%d, zoom %g" % (x, zoom))
    for label, node in converters:
        row = displayed_row(node, zoom, xs)
        print("zoom %g, %s: %r" % (zoom, label, [c[0] for c in row]))
        for x, c, r in zip(xs, row, reference):
            for k, name in enumerate(("red", "green", "blue")):
                if abs(c[k] - r[k]) > TOLERANCE:
                    failures.append("%s at zoom %g, x=%d: %s %d, float ramp %d" % (label, zoom, x, name, c[k], r[k]))
check(not failures, "8/16-bit images differ from the float ramp:\n  " + "\n  ".join(failures))
