# GuiApp.saveScreenshot() saves what the viewer displays: an image with an alpha below 1 must give an
# opaque screenshot with the displayed colour. The viewer's framebuffer keeps the image's alpha (it is
# drawn without blending when the checkerboard is off), and the screenshot used to carry it, so a
# semi-transparent image came out semi-transparent, with other colours (#1549).
import os
import tempfile

import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtGui
except ImportError:
    from PySide6 import QtGui

require_plugin(GENERATOR)  # fills R, G, B and A with 0.5

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
viewer.connectInput(0, gen)
fit_viewer()
app.getViewer("Viewer1").renderCurrentFrame(True)
wait_idle()
x, y = rod_center(gen)
displayed = NatronGuiTest.getDisplayedPixel("Viewer1", x, y)
print("displayed at the centre: %r" % (displayed,))
check_close(displayed[0], srgb8(0.5), 3, "displayed red")

fd, path = tempfile.mkstemp(suffix=".png", prefix="natron-screenshot-")
os.close(fd)
try:
    check(app.saveScreenshot(path, "Viewer1"), "saveScreenshot() failed")
    shot = QtGui.QImage(path)
    check(not shot.isNull(), "cannot read the screenshot %s" % path)
    shot = shot.convertToFormat(QtGui.QImage.Format_ARGB32)
    transparent = 0
    found = 0
    for j in range(0, shot.height(), 2):
        for i in range(0, shot.width(), 2):
            c = QtGui.QColor.fromRgba(shot.pixel(i, j))
            if c.alpha() != 255:
                transparent += 1
            elif abs(c.red() - displayed[0]) <= 2 and abs(c.green() - displayed[1]) <= 2 and abs(c.blue() - displayed[2]) <= 2:
                found += 1
    print("screenshot %dx%d: %d non-opaque samples, %d samples with the displayed colour" % (shot.width(), shot.height(), transparent, found))
    check(transparent == 0, "the screenshot has %d non-opaque samples" % transparent)
    check(found > 0, "the screenshot does not contain the displayed colour %r" % (displayed[:3],))
finally:
    os.remove(path)
