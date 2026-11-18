.. _guiTesting:

Testing Natron's GUI from scripts
=================================

A Python script can drive the graphical interface of Natron and check the result, e.g. to
write a regression test for a viewer overlay or to make screenshots for the documentation.
Four features make this possible:

- the :ref:`--no-dialogs<natronExec>` command-line option: the message and question dialogs,
  including the startup prompts, are not shown, they return their default button and are
  printed to the standard error, so that nothing blocks the script;
- :meth:`natron.quit(exitCode)<NatronEngine.PyCoreApplication.quit>`: closes the projects
  without asking to save them and exits with *exitCode*, so that the result of the test is
  the exit status of Natron;
- :meth:`app.saveScreenshot(filePath, scriptName)<NatronGui.GuiApp.saveScreenshot>`: saves an
  image of the main window, or of a tab or pane, including the content of the viewers and of
  the other OpenGL widgets. Only the widgets of Natron are captured, never the screen;
- the :ref:`input functions of PyViewer<pyViewer>`, such as
  :meth:`drag<NatronGui.PyViewer.drag>` and :meth:`keyPress<NatronGui.PyViewer.keyPress>`:
  they send real mouse and key events to a viewer, in canonical coordinates (the coordinates
  the overlays use, y pointing up), so that the overlays of the nodes react as they would to
  the user. They work when the window is not on top, and they do not move the mouse cursor.

How the script runs
-------------------

Natron runs the script given on the command line while it creates the project, before the
event loop starts and before the main window is shown on screen. Do the test from a timer, so
that it runs once the GUI is up::

    Natron --no-dialogs --no-settings test_transform.py

*--no-settings* uses the default preferences, so that the test does not depend on the user's
preferences. In the script, *app* is the :doc:`GuiApp<PythonReference/NatronGui/GuiApp>` of the
project, and the PySide module that Natron uses (PySide2 with Qt 5, PySide6 with Qt 6) is
available.

When Natron runs a script, it does not create a viewer: create one with
``app.createNode("fr.inria.built-in.Viewer")``.
The viewer only shows the overlay of a node, and only sends it the events, while the settings
panel of the node is open, and :meth:`createNode<NatronEngine.App.createNode>` does not open
it: select the node and send the *Return* key to the node graph, as in the example below.

Example: dragging the Transform handle
--------------------------------------

This script creates Read → Transform → Viewer1, drags the centre handle of the Transform
overlay by (100, 50), checks the *translate* parameter, undoes the change, saves screenshots,
and exits with 0 on success and 1 on failure. It needs the openfx-io and openfx-misc plug-ins
(set *OFX_PLUGIN_PATH* if they are not installed), and an image to read::

    import sys
    from NatronGui import natron
    try:
        from PySide2 import QtCore, QtGui, QtWidgets
    except ImportError:
        from PySide6 import QtCore, QtGui, QtWidgets

    IMAGE = "/path/to/image.png"
    OUT = "/tmp"

    def openSettingsPanel(app, effect):
        # as selecting the node and pressing Return in the node graph
        app.selectNode(effect, True)
        for w in QtWidgets.QApplication.allWidgets():
            if w.metaObject().className().endswith("NodeGraph") and w.isVisible():
                for t in (QtCore.QEvent.KeyPress, QtCore.QEvent.KeyRelease):
                    QtWidgets.QApplication.sendEvent(w, QtGui.QKeyEvent(t, QtCore.Qt.Key_Return, QtCore.Qt.NoModifier))

    def undo():
        # Edit > Undo
        for w in QtWidgets.QApplication.topLevelWidgets():
            if isinstance(w, QtWidgets.QMainWindow):
                for m in w.menuBar().actions():
                    for a in (m.menu().actions() if m.menu() else []):
                        if a.shortcut() == QtGui.QKeySequence(QtGui.QKeySequence.Undo) and a.isEnabled():
                            a.trigger()
                            return

    def test():
        read = app.createNode("fr.inria.built-in.Read")
        read.getParam("filename").set(IMAGE)
        transform = app.createNode("net.sf.openfx.TransformPlugin")
        transform.connectInput(0, read)
        viewerNode = app.createNode("fr.inria.built-in.Viewer")  # Viewer1
        viewerNode.connectInput(0, transform)

        viewer = app.getViewer("Viewer1")
        viewer.fitToFormat()
        viewer.waitForRender()
        app.saveScreenshot(OUT + "/mainWindow.png")
        app.saveScreenshot(OUT + "/Viewer1.png", "Viewer1")

        openSettingsPanel(app, transform)
        viewer.waitForRender()
        translate = transform.getParam("translate")
        center = transform.getParam("center")
        t0 = translate.get()
        cx = center.get().x + t0.x
        cy = center.get().y + t0.y
        viewer.drag(cx, cy, cx + 100, cy + 50)
        viewer.waitForRender()
        t1 = translate.get()
        tol = 1.5 / viewer.getZoom()  # the overlay moves by whole screen pixels
        if abs(t1.x - t0.x - 100) > tol or abs(t1.y - t0.y - 50) > tol:
            return 1

        undo()
        if translate.get().x != t0.x:
            return 1

        # a viewer shortcut: + zooms in
        zoom = viewer.getZoom()
        viewer.keyPress(QtCore.Qt.Key_Plus)
        viewer.processEvents()
        if viewer.getZoom() <= zoom:
            return 1
        return 0

    def main():
        status = 1
        try:
            status = test()
        finally:
            natron.quit(status)

    QtCore.QTimer.singleShot(500, main)

Run it and check the exit status::

    Natron --no-dialogs --no-settings test_transform.py; echo "exit status: $?"

The GUI smoke test of the Natron sources, ``Tests/gui/run_gui_smoke_test.py``, is a longer
version of this example that also checks that the screenshots are not black. CTest runs it as
*GuiSmokeTest*; it is skipped (exit status 77) when there is no display, in CI unless
*NATRON_GUI_TESTS=1* is set, or when the plug-ins are missing.

Tips
----

- Wait for the viewer with :meth:`waitForRender<NatronGui.PyViewer.waitForRender>` after
  changing the graph, the view or a parameter, before checking values or saving a screenshot.
- The overlays snap the pointer to the screen pixels: compare the values with a tolerance of
  about one pixel, i.e. ``1.5 / viewer.getZoom()`` in canonical units.
- In the GUI, the output of the script (``print``) goes to the Script Editor. To see it in the
  terminal, write to ``sys.__stderr__``.
- With some OpenFX plug-ins, Natron may crash at exit, after the projects were closed, which
  changes the exit status (see `issue #1253 <https://github.com/NatronGitHub/Natron/issues/1253>`_).
  To be safe, also write the result of the test to a file before calling
  :meth:`natron.quit()<NatronEngine.PyCoreApplication.quit>`.
- :meth:`natron.quit()<NatronEngine.PyCoreApplication.quit>` also works with
  *NatronRenderer*, e.g. to make a rendering script exit with an error status.
