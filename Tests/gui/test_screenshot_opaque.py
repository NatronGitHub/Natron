"""GuiApp.saveScreenshot() saves what the viewer displays.

An image whose alpha is below 1 must give an opaque screenshot with the displayed colour. The viewer
draws its image without blending when the checkerboard is off, so its framebuffer keeps the image's
alpha, and the screenshot used to carry it: a semi-transparent image came out semi-transparent, with
other colours (#1549).
"""
import os
import tempfile

import NatronGuiTest
from natrontest import *

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    from PySide6 import QtCore, QtGui, QtWidgets

require_plugin(GENERATOR)  # fills R, G, B and A with 0.5


def viewer_gl_and_tab():
    """The OpenGL widget of Viewer1 and the ViewerTab that contains it."""
    gls = [w for w in QtWidgets.QApplication.allWidgets()
           if isinstance(w, QtWidgets.QOpenGLWidget) and w.objectName() == "Viewer1"]
    check(len(gls) == 1, "found %d OpenGL widgets named Viewer1" % len(gls))
    tab = gls[0].parentWidget()
    while tab is not None and tab.metaObject().className() != "Natron::ViewerTab":
        tab = tab.parentWidget()
    check(tab is not None, "no ViewerTab above the Viewer1 OpenGL widget")
    return gls[0], tab


def close(c, rgb, tol):
    """Whether the QColor c is within tol of the (r, g, b) tuple rgb."""
    return all(abs(a - b) <= tol for a, b in zip((c.red(), c.green(), c.blue()), rgb))


viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
viewer.connectInput(0, gen)
fit_viewer()
app.getViewer("Viewer1").renderCurrentFrame(True)
wait_idle()
x, y = rod_center(gen)
displayed = NatronGuiTest.getDisplayedPixel("Viewer1", x, y)
print("displayed at the centre of the image: %r" % (displayed,))
check_close(displayed[0], srgb8(0.5), 3, "displayed red")

gl, tab = viewer_gl_and_tab()
fd, path = tempfile.mkstemp(suffix=".png", prefix="natron-screenshot-")
os.close(fd)
try:
    check(app.saveScreenshot(path, "Viewer1"), "saveScreenshot() failed")
    shot = QtGui.QImage(path)
    check(not shot.isNull(), "cannot read the screenshot %s" % path)
    shot = shot.convertToFormat(QtGui.QImage.Format_ARGB32)
    # the centre of the viewer, which fit_viewer() filled with the image, in screenshot pixels
    scale = shot.width() / float(tab.width())
    centre = gl.mapTo(tab, QtCore.QPoint(gl.width() // 2, gl.height() // 2))
    c = QtGui.QColor.fromRgba(shot.pixel(int(centre.x() * scale), int(centre.y() * scale)))
    print("screenshot %dx%d, viewer centre: %r" % (shot.width(), shot.height(), c.getRgb()))
    check(c.alpha() == 255, "the screenshot is not opaque at the viewer centre: alpha %d" % c.alpha())
    check(close(c, displayed[:3], 2),
          "the screenshot shows %r at the viewer centre, the viewer %r" % (c.getRgb()[:3], displayed[:3]))
    transparent = 0
    for j in range(0, shot.height(), 2):
        for i in range(0, shot.width(), 2):
            if QtGui.QColor.fromRgba(shot.pixel(i, j)).alpha() != 255:
                transparent += 1
    check(transparent == 0, "the screenshot has %d non-opaque samples" % transparent)
finally:
    os.remove(path)
