"""Dragging the tab of an OpenGL panel to another pane or to a floating window keeps it drawn (#536).

On Linux with X11 and Mesa, Natron 2.3 to 2.5 exited with a GLX error (BadValue in X_GLXCreateContext)
as soon as the Curve Editor tab was dragged. On macOS, the axis labels of a floating Curve Editor were
drawn as solid rectangles: moving a QOpenGLWidget to another top-level window recreated its OpenGL
context, and the text renderer kept using the glyph textures of the old one. Natron now shares the
contexts of its OpenGL widgets (Qt::AA_ShareOpenGLContexts), so that a widget keeps its context when it
moves, and the widgets release and recreate their OpenGL objects if their context is replaced.

The test drags the tabs the way the user does (press on the tab, move, release), with synthetic mouse
events sent to the tab bar, which grabs the mouse during a tab drag: the Curve Editor into the viewer's
pane, then each of the Curve Editor, the Dope Sheet and the viewer out of the main window (a floating
window) and back into the Node Graph's pane. After each drop, the panel's OpenGL view must be shown and
drawn in its new window: the Curve Editor and the Dope Sheet with their scale labels drawn as text, not
as solid rectangles (the test sets their scale colour to magenta to find the labels), the viewer with
its image.

A regression guard for the drag, float and dock path: Mesa llvmpipe under Xvfb may not reproduce the
driver-specific GLX error of the report.
"""
import os
import tempfile

import NatronGuiTest
from natrontest import *

QtCore, QtGui, QtWidgets = qt_modules()

CURVE_EDITOR = "Curve Editor"  # the tab labels
DOPE_SHEET = "Dope Sheet"
VIEWER = "Viewer1"
TAB_BAR_NAME = "dragTabBar"  # given by this test to the tab bar of the dragged tab
LABEL_RGB = (255, 0, 255)  # the scale colour set by this test: no other drawing of these views is magenta
VIEWER_TEXT_RGB = (200, 200, 200)  # the colour of the viewer's text (ViewerGL::Implementation::textRenderingColor)

# for each tab: the class of its OpenGL view, and its name for GuiApp.saveScreenshot()
PANELS = {
    CURVE_EDITOR: ("Natron::CurveWidget", "curveEditor"),
    DOPE_SHEET: ("Natron::DopeSheetView", "dopeSheetEditor"),
    VIEWER: ("Natron::ViewerGL", VIEWER),
}


def main_window():
    wins = [w for w in QtWidgets.QApplication.topLevelWidgets() if w.metaObject().className() == "Natron::Gui"]
    check(len(wins) == 1, "found %d main windows" % len(wins))
    return wins[0]


def pane_widgets():
    return [w for w in QtWidgets.QApplication.allWidgets() if w.metaObject().className() == "Natron::TabWidget"]


def pane_widget(name):
    panes = [w for w in pane_widgets() if w.objectName() == name]
    check(len(panes) == 1, "found %d panes named %s" % (len(panes), name))
    return panes[0]


def pane_of(label):
    """The script name of the pane (GuiApp.getTabWidget()) that has the tab label, and the index of the tab."""
    found = []
    for name in sorted(w.objectName() for w in pane_widgets()):
        p = app.getTabWidget(name)
        if p is not None:
            labels = [p.getTabLabel(j) for j in range(p.count())]
            if label in labels:
                found.append((name, labels.index(label)))
    check(len(found) == 1, "the %s tab is in %d panes: %r" % (label, len(found), found))
    return found[0]


def show_tab(label):
    """Makes the tab label the current tab of its pane."""
    name, index = pane_of(label)
    app.getTabWidget(name).setCurrentIndex(index)
    wait_idle()


def gl_view(label):
    """The panel of the tab label (the widget in its pane) and the panel's OpenGL view."""
    gl_class = PANELS[label][0]
    gls = [w for w in QtWidgets.QApplication.allWidgets() if w.metaObject().className() == gl_class]
    if label == VIEWER:
        gls = [w for w in gls if w.objectName() == VIEWER]
    check(len(gls) == 1, "found %d %s widgets" % (len(gls), gl_class))
    panel = gls[0]
    while panel is not None and (panel.parentWidget() is None
                                 or panel.parentWidget().metaObject().className() != "Natron::TabWidget"):
        panel = panel.parentWidget()
    check(panel is not None, "the %s is in no pane" % gl_class)
    return panel, gls[0]


def tab_bar(label):
    """The visible tab bar that has the tab label, and the index of that tab; names the tab bar TAB_BAR_NAME.

    Tab bars have no name of their own: the previous one loses the name, so that NatronGuiTest finds
    the right one.
    """
    found = []
    for w in QtWidgets.QApplication.allWidgets():
        if w.inherits("QTabBar") and w.isVisible():
            if w.objectName() == TAB_BAR_NAME:
                w.setObjectName("")
            labels = [w.tabText(i) for i in range(w.count())]
            if label in labels:
                found.append((w, labels.index(label)))
    check(len(found) == 1, "found %d visible tab bars with the %s tab" % (len(found), label))
    bar, index = found[0]
    bar.setObjectName(TAB_BAR_NAME)
    return bar, index


def drag_tab_to(label, global_pos, steps=20):
    """Drags the tab label to the global position global_pos (a QPoint), as the user does."""
    bar, index = tab_bar(label)
    start = bar.tabRect(index).center()
    end = bar.mapFromGlobal(global_pos)
    print("dragging the %s tab from %r to global %r" % (label, (start.x(), start.y()), (global_pos.x(), global_pos.y())))
    NatronGuiTest.penEvent(TAB_BAR_NAME, "move", start.x(), start.y(), "none")
    NatronGuiTest.penEvent(TAB_BAR_NAME, "press", start.x(), start.y())
    # still on the tab, but further than QApplication::startDragDistance(): the tab drag starts
    # (TabBar::mouseMoveEvent() drags the tab under that move)
    x0 = start.x() + QtWidgets.QApplication.startDragDistance() + 2
    NatronGuiTest.penEvent(TAB_BAR_NAME, "move", x0, start.y())
    for i in range(1, steps + 1):
        NatronGuiTest.penEvent(TAB_BAR_NAME, "move", x0 + (end.x() - x0) * i / steps,
                               start.y() + (end.y() - start.y()) * i / steps)
    NatronGuiTest.penEvent(TAB_BAR_NAME, "release", end.x(), end.y())
    wait_idle()


def view_pixels(label, what):
    """The OpenGL view of the tab label as drawn on screen: rows of (r, g, b) in device pixels, and the
    number of device pixels per widget pixel.

    GuiApp.saveScreenshot() draws each OpenGL widget from its framebuffer (QOpenGLWidget::grabFramebuffer()),
    which renders it again. The test doesn't call grabFramebuffer() itself: Qt 6 moved QOpenGLWidget to
    QtOpenGLWidgets, which CI's PySide6 packages don't install.
    """
    panel, gl = gl_view(label)
    check(panel.isVisible() and gl.isVisible(), "the %s is hidden after %s" % (label, what))
    fd, path = tempfile.mkstemp(suffix=".png", prefix="natron-gl-panel-")
    os.close(fd)
    try:
        check(app.saveScreenshot(path, PANELS[label][1]), "cannot take a screenshot of the %s after %s" % (label, what))
        shot = QtGui.QImage(path)
    finally:
        os.remove(path)
    check(not shot.isNull() and shot.width() > 0 and panel.width() > 0,
          "empty screenshot of the %s after %s" % (label, what))
    scale = shot.width() / panel.width()
    pos = gl.mapTo(panel, QtCore.QPoint(0, 0))
    img = shot.copy(round(pos.x() * scale), round(pos.y() * scale),
                    round(gl.width() * scale), round(gl.height() * scale)).convertToFormat(QtGui.QImage.Format_RGB32)
    rows = []
    for y in range(img.height()):
        row = []
        for x in range(img.width()):
            p = img.pixel(x, y)
            row.append(((p >> 16) & 0xff, (p >> 8) & 0xff, p & 0xff))
        rows.append(row)
    return rows, scale


def solid_block(rows, is_text, min_w, min_h):
    """The (x, y, w, h) of a block of at least min_w x min_h pixels of one colour that is_text(), or None.

    A glyph drawn without its texture is a solid rectangle of the text colour, as wide as the advance of
    the character and as high as the font. The strokes of a glyph drawn from its texture are thinner than
    min_w, and anti-aliased.
    """
    height, width = len(rows), len(rows[0]) if rows else 0
    run = [0] * width  # for each column, the height of the run of one text colour that ends at row y
    for y in range(height):
        row = rows[y]
        for x in range(width):
            c = row[x]
            if not is_text(c):
                run[x] = 0
            elif run[x] > 0 and rows[y - 1][x] == c:
                run[x] += 1
            else:
                run[x] = 1
        x = 0
        while x < width:
            if run[x] < min_h:
                x += 1
                continue
            x1 = x + 1
            while x1 < width and run[x1] >= min_h and row[x1] == row[x]:
                x1 += 1
            if x1 - x >= min_w:
                return (x, y - min_h + 1, x1 - x, min_h)
            x = x1
    return None


def is_label(c):
    """Whether the pixel colour c is LABEL_RGB (magenta), possibly blended with a grey background."""
    r, g, b = c
    return abs(r - b) <= 3 and r - g >= 40


def text_pixels(rows, is_text):
    """The number of pixels that is_text(), outside the rows and columns of grid lines (mostly is_text())."""
    height, width = len(rows), len(rows[0]) if rows else 0
    marks = [[is_text(c) for c in row] for row in rows]
    lines_x = set(x for x in range(width) if sum(marks[y][x] for y in range(height)) > height // 2)
    lines_y = set(y for y in range(height) if sum(marks[y]) > width // 2)
    return sum(1 for y in range(height) if y not in lines_y
               for x in range(width) if marks[y][x] and x not in lines_x)


def check_view_drawn(label, what):
    """The OpenGL view of the tab label is shown and drawn: its labels as text, the viewer's image."""
    if label == VIEWER:
        fit_viewer()  # the image fills the viewer, whatever its new size
    rows, scale = view_pixels(label, what)
    height, width = len(rows), len(rows[0]) if rows else 0
    colours = set(c for row in rows for c in row)
    check(len(colours) > 1, "the %s is a uniform %r after %s" % (label, next(iter(colours), None), what))
    min_w, min_h = round(4 * scale), round(8 * scale)
    if label == VIEWER:
        # the Generator's image: its colour at the centre
        c = rows[height // 2][width // 2]
        check(all(abs(v - srgb8(0.5)) <= 3 for v in c),
              "the viewer shows %r at its centre after %s, not the image (%d)" % (c, what, srgb8(0.5)))
        block = solid_block(rows, lambda p: p == VIEWER_TEXT_RGB, min_w, min_h)
    else:
        n = text_pixels(rows, is_label)
        print("%s after %s: %dx%d device pixels, %d label pixels" % (label, what, width, height, n))
        check(n >= 20, "the %s has no scale labels after %s (%d label pixels)" % (label, what, n))
        block = solid_block(rows, is_label, min_w, min_h)
    check(block is None, "the %s draws a solid rectangle %r of the text colour %r after %s: a glyph without its texture"
          % (label, block, block and rows[block[1]][block[0]], what))


def floating_windows():
    return [w for w in QtWidgets.QApplication.topLevelWidgets()
            if w.metaObject().className() == "Natron::FloatingWidget" and w.isVisible()]


def free_point(screen, main):
    """A point of the screen, right of the main window, over no widget of Natron (where a drop floats the tab)."""
    frame = main.frameGeometry()
    for x in range(frame.right() + 20, screen.right() - 20, 40):
        for y in range(screen.top() + 20, screen.bottom() - 100, 80):
            p = QtCore.QPoint(x, y)
            if QtWidgets.QApplication.widgetAt(p) is None:
                return p
    return None


def float_and_dock(label, home):
    """Drags the tab label out of the main window (a floating window), then into the pane named home."""
    screen = QtGui.QGuiApplication.primaryScreen().geometry()
    drop_pos = free_point(screen, main)
    if drop_pos is None:
        # no free point on the screen: off the screen, where there is no widget either
        drop_pos = QtCore.QPoint(screen.left() - 50, screen.top() - 50)
    drag_tab_to(label, drop_pos)
    floating = gl_view(label)[0].window()
    check(floating is not main, "the %s tab dropped outside the main window did not float" % label)
    check(floating.metaObject().className() == "Natron::FloatingWidget",
          "the %s is in a %s, not in a floating window" % (label, floating.metaObject().className()))
    check(floating.isVisible(), "the floating window of the %s is hidden" % label)
    check_view_drawn(label, "the drag out of the main window")

    # back into the main window: the floating window, now empty, closes
    pane = pane_widget(home)
    drag_tab_to(label, pane.mapToGlobal(pane.rect().center()))
    check(gl_view(label)[0].window() is main, "the %s did not go back to the main window" % label)
    check(wait_until(lambda: not floating_windows(), 10), "the empty floating window of the %s is still shown" % label)
    check_view_drawn(label, "the drag back into the main window")


# the scale labels in a colour that no other drawing of these views has
settings = natron.getSettings()
for name in ("curveeditorScale", "dopesheetScale"):
    param = settings.getParam(name)
    check(param is not None, "no %s setting" % name)
    for dim, v in enumerate(LABEL_RGB):
        param.setValue(v / 255.0, dim)

# an image for the viewer
require_plugin(GENERATOR)
gen = app.createNode(GENERATOR)
viewer_node(app).connectInput(0, gen)
wait_idle()

main = main_window()
workshop = pane_of(CURVE_EDITOR)[0]  # pane3 in the default layout, with the Node Graph and the Dope Sheet
viewer_pane = pane_of(VIEWER)[0]
check(workshop != viewer_pane, "the %s tab is in the viewer's pane" % CURVE_EDITOR)
check(pane_of(DOPE_SHEET)[0] == workshop, "the %s tab is not in the pane of the %s" % (DOPE_SHEET, CURVE_EDITOR))

# room right of the main window on the screen, for the floating windows
screen = QtGui.QGuiApplication.primaryScreen().geometry()
print("screen %r, main window %r" % (screen.getRect(), main.frameGeometry().getRect()))
if free_point(screen, main) is None:
    main.setGeometry(screen.x(), screen.y(), screen.width() * 2 // 3, screen.height())
    wait_idle()
    print("main window resized to %r" % (main.frameGeometry().getRect(),))

# docked, before any drag: the checks pass when the views are drawn right
for label in (CURVE_EDITOR, DOPE_SHEET, VIEWER):
    show_tab(label)
    check_view_drawn(label, "the start")

# 1. the Curve Editor into another pane: the middle of the viewer's pane (the tab is added to it, or to a
#    new pane next to it if the drop is near the pane's border)
pane = pane_widget(viewer_pane)
drag_tab_to(CURVE_EDITOR, pane.mapToGlobal(pane.rect().center()))
check(pane_of(CURVE_EDITOR)[0] != workshop, "the %s tab is still in %s" % (CURVE_EDITOR, workshop))
check(gl_view(CURVE_EDITOR)[0].window() is main, "the %s left the main window" % CURVE_EDITOR)
check_view_drawn(CURVE_EDITOR, "the drag to the viewer's pane")

# 2. each panel out of the main window, and back into the Node Graph's pane; the viewer last, because
#    it leaves its pane empty
for label in (CURVE_EDITOR, DOPE_SHEET, VIEWER):
    float_and_dock(label, workshop)

# Natron is still alive and responsive
app.getViewer(VIEWER).renderCurrentFrame(True)
wait_idle()
check(app.getNode(gen.getScriptName()) is not None, "the project lost its node")
print("the %s, %s and %s tabs were floated and docked again without a crash" % (CURVE_EDITOR, DOPE_SHEET, VIEWER))
