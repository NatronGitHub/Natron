"""Dragging the Curve Editor tab to another pane and to a floating window must not crash (#536).

On Linux with X11 and Mesa, Natron 2.3 to 2.5 exited with a GLX error (BadValue in X_GLXCreateContext)
as soon as the Curve Editor tab was dragged: moving the curve editor's OpenGL widget to another
window recreates its OpenGL context. The test drags the tab the way the user does (press on the tab,
move, release), with synthetic mouse events sent to the tab bar, which grabs the mouse during a tab
drag: into the viewer's pane, out of the main window (a floating window), and back into the Node
Graph's pane. After each drop, the curve editor must be shown and drawn (its screenshot is taken with
its OpenGL content) in its new window.
"""
import os
import tempfile

import NatronGuiTest
from natrontest import *

QtCore, QtGui, QtWidgets = qt_modules()

CURVE_EDITOR = "Curve Editor"  # the tab label
TAB_BAR_NAME = "curveEditorTabBar"  # given by this test to the tab bar that has the Curve Editor tab


def main_window():
    wins = [w for w in QtWidgets.QApplication.topLevelWidgets() if w.metaObject().className() == "Natron::Gui"]
    check(len(wins) == 1, "found %d main windows" % len(wins))
    return wins[0]


def curve_editor():
    eds = [w for w in QtWidgets.QApplication.allWidgets() if w.objectName() == "CurveEditor"]
    check(len(eds) == 1, "found %d curve editors" % len(eds))
    return eds[0]


def pane_widget(name):
    panes = [w for w in QtWidgets.QApplication.allWidgets()
             if w.objectName() == name and w.metaObject().className() == "Natron::TabWidget"]
    check(len(panes) == 1, "found %d panes named %s" % (len(panes), name))
    return panes[0]


def curve_editor_tab_bar():
    """The tab bar that has the Curve Editor tab, and the index of that tab; names the tab bar TAB_BAR_NAME.

    Tab bars have no name of their own: the previous one loses the name, so that NatronGuiTest finds
    the right one.
    """
    found = []
    for w in QtWidgets.QApplication.allWidgets():
        if w.inherits("QTabBar") and w.isVisible():
            if w.objectName() == TAB_BAR_NAME:
                w.setObjectName("")
            labels = [w.tabText(i) for i in range(w.count())]
            if CURVE_EDITOR in labels:
                found.append((w, labels.index(CURVE_EDITOR)))
    check(len(found) == 1, "found %d visible tab bars with the %s tab" % (len(found), CURVE_EDITOR))
    bar, index = found[0]
    bar.setObjectName(TAB_BAR_NAME)
    return bar, index


def pane_of_curve_editor():
    """The script name of the pane (GuiApp.getTabWidget()) that has the Curve Editor tab."""
    names = []
    for i in range(1, 30):
        name = "pane%d" % i
        p = app.getTabWidget(name)
        if p is not None and CURVE_EDITOR in [p.getTabLabel(j) for j in range(p.count())]:
            names.append(name)
    check(len(names) == 1, "the %s tab is in %d panes: %r" % (CURVE_EDITOR, len(names), names))
    return names[0]


def drag_tab_to(globalPos, steps=20):
    """Drag the Curve Editor tab to the global position globalPos (a QPoint), as the user does."""
    bar, index = curve_editor_tab_bar()
    start = bar.tabRect(index).center()
    end = bar.mapFromGlobal(globalPos)
    print("dragging the %s tab from %r to global %r" % (CURVE_EDITOR, (start.x(), start.y()), (globalPos.x(), globalPos.y())))
    NatronGuiTest.penEvent(TAB_BAR_NAME, "move", start.x(), start.y(), "none")
    NatronGuiTest.penEvent(TAB_BAR_NAME, "press", start.x(), start.y())
    # still on the tab, but further than QApplication::startDragDistance() (10): the tab drag starts
    NatronGuiTest.penEvent(TAB_BAR_NAME, "move", start.x() + 12, start.y())
    x0 = start.x() + 12
    for i in range(1, steps + 1):
        NatronGuiTest.penEvent(TAB_BAR_NAME, "move", x0 + (end.x() - x0) * i / float(steps),
                               start.y() + (end.y() - start.y()) * i / float(steps))
    NatronGuiTest.penEvent(TAB_BAR_NAME, "release", end.x(), end.y())
    wait_idle()


def check_curve_editor_drawn(what):
    """The curve editor is shown, and its screenshot (which reads its OpenGL framebuffer) can be saved."""
    ed = curve_editor()
    check(ed.isVisible(), "the curve editor is hidden after %s" % what)
    gls = [w for w in ed.findChildren(QtWidgets.QWidget) if w.inherits("QOpenGLWidget")]
    check(len(gls) == 1 and gls[0].isVisible(), "the curve editor's OpenGL widget is not shown after %s" % what)
    gls[0].update()
    wait_idle()
    fd, path = tempfile.mkstemp(suffix=".png", prefix="natron-curve-editor-")
    os.close(fd)
    try:
        check(app.saveScreenshot(path, "curveEditor"), "cannot take a screenshot of the curve editor after %s" % what)
        shot = QtGui.QImage(path)
        check(not shot.isNull() and shot.width() > 0, "empty screenshot of the curve editor after %s" % what)
    finally:
        os.remove(path)


def free_point(screen, main):
    """A point of the screen, right of the main window, over no widget of Natron (where a drop floats the tab)."""
    frame = main.frameGeometry()
    for x in range(screen.right() - 20, frame.right(), -40):
        for y in range(screen.top() + 20, screen.bottom() - 100, 80):
            p = QtCore.QPoint(x, y)
            if QtWidgets.QApplication.widgetAt(p) is None:
                return p
    return None


# an animated parameter, so that the curve editor has a curve to draw
require_plugin(GENERATOR)
gen = app.createNode(GENERATOR)
viewer_node(app).connectInput(0, gen)
wait_idle()

main = main_window()
workshop = pane_of_curve_editor()  # pane3 in the default layout, with the Node Graph and the Dope Sheet
viewerPane = app.getTabWidget("pane1")
check(viewerPane is not None and "Viewer1" in [viewerPane.getTabLabel(j) for j in range(viewerPane.count())],
      "pane1 doesn't have the viewer")
check(workshop != "pane1", "the %s tab is in the viewer's pane" % CURVE_EDITOR)

# 1. into another pane: the middle of the viewer's pane (the tab is added to it, or to a new pane next to it
#    if the drop is near the pane's border)
viewerPaneWidget = pane_widget("pane1")
drag_tab_to(viewerPaneWidget.mapToGlobal(viewerPaneWidget.rect().center()))
check(pane_of_curve_editor() != workshop, "the %s tab is still in %s" % (CURVE_EDITOR, workshop))
check(curve_editor().window() is main, "the curve editor left the main window")
check_curve_editor_drawn("the drag to the viewer's pane")

# 2. out of the main window: a floating window. Make room right of the main window on the screen.
screen = QtGui.QGuiApplication.primaryScreen().geometry()
print("screen %r, main window %r" % (screen.getRect(), main.frameGeometry().getRect()))
if free_point(screen, main) is None:
    main.setGeometry(screen.x(), screen.y(), screen.width() * 2 // 3, screen.height())
    wait_idle()
    print("main window resized to %r" % (main.frameGeometry().getRect(),))
dropPos = free_point(screen, main)
if dropPos is None:
    # no free point on the screen: off the screen, where there is no widget either
    dropPos = QtCore.QPoint(screen.left() - 50, screen.top() - 50)
drag_tab_to(dropPos)
floating = curve_editor().window()
check(floating is not main, "the %s tab dropped outside the main window did not float" % CURVE_EDITOR)
check(floating.metaObject().className() == "Natron::FloatingWidget",
      "the curve editor is in a %s, not in a floating window" % floating.metaObject().className())
check(floating.isVisible(), "the floating window is hidden")
check_curve_editor_drawn("the drag out of the main window")

# 3. back into the main window, over the Node Graph's pane: the floating window, now empty, closes
graph = pane_widget(workshop)
drag_tab_to(graph.mapToGlobal(graph.rect().center()))
check(curve_editor().window() is main, "the curve editor did not go back to the main window")
check(wait_until(lambda: not any(w.metaObject().className() == "Natron::FloatingWidget" and w.isVisible()
                                 for w in QtWidgets.QApplication.topLevelWidgets()), 10),
      "the empty floating window is still shown")
check_curve_editor_drawn("the drag back into the main window")

# Natron is still alive and responsive
app.getViewer("Viewer1").renderCurrentFrame(True)
wait_idle()
check(app.getNode(gen.getScriptName()) is not None, "the project lost its node")
print("the %s tab was dragged to pane1, to a floating window and back without a crash" % CURVE_EDITOR)
