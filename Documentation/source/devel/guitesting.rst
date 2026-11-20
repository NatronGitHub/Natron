.. _guiTesting:

Testing Natron's GUI from scripts
=================================

A Python script can drive the graphical interface of Natron and check the result, e.g. to
write a regression test or to make screenshots for the documentation. Three features make this
possible:

- the :option:`--no-dialogs` command-line option: the message and question dialogs,
  including the startup prompts, are not shown, they return their default button and are
  printed to the standard error, so that nothing blocks the script. "Save changes?" is answered
  Discard and the file dialogs count as cancelled (see the list of answers in
  :option:`--no-dialogs`);
- :meth:`natron.quit(exitCode)<NatronEngine.PyCoreApplication.quit>`: closes the projects
  without asking to save them and exits with *exitCode*, so that the result of the test is
  the exit status of Natron;
- :meth:`app.saveScreenshot(filePath, scriptName)<NatronGui.GuiApp.saveScreenshot>`: saves an
  image of the main window, or of a tab or pane, including the content of the viewers and of
  the other OpenGL widgets. Only the widgets of Natron are captured, never the screen.

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

Example: a screenshot of the viewer
-----------------------------------

This script creates Read → Transform → Viewer1, waits for the viewer, saves screenshots and
exits with 0 on success and 1 on failure. It needs the openfx-io and openfx-misc plug-ins (set
*OFX_PLUGIN_PATH* if they are not installed), and an image to read::

    from NatronGui import natron
    try:
        from PySide2 import QtCore
    except ImportError:
        from PySide6 import QtCore

    IMAGE = "/path/to/image.png"
    OUT = "/tmp"

    def setUp():
        try:
            read = app.createNode("fr.inria.built-in.Read")
            read.getParam("filename").set(IMAGE)
            transform = app.createNode("net.sf.openfx.TransformPlugin")
            transform.connectInput(0, read)
            viewerNode = app.createNode("fr.inria.built-in.Viewer")  # Viewer1
            viewerNode.connectInput(0, transform)
            app.getViewer("Viewer1").renderCurrentFrame(True)
        except Exception:
            natron.quit(1)
            return
        # let the viewer render and redraw, then check
        QtCore.QTimer.singleShot(3000, check)

    def check():
        status = 1
        try:
            ok = app.saveScreenshot(OUT + "/mainWindow.png")
            ok = app.saveScreenshot(OUT + "/Viewer1.png", "Viewer1") and ok
            status = 0 if ok else 1
        finally:
            natron.quit(status)  # the last call of the callback

    QtCore.QTimer.singleShot(500, setUp)

Run it and check the exit status::

    Natron --no-dialogs --no-settings test_screenshot.py; echo "exit status: $?"

The GUI smoke test of the Natron sources, ``Tests/gui/run_gui_smoke_test.py``, is a longer
version of this example that also checks that the screenshots are not black, and that
quitting with :meth:`natron.quit()<NatronEngine.PyCoreApplication.quit>` or by closing the main
window does not block on "Save changes?" and leaves no auto-save behind. CTest runs it as
*GuiSmokeTest*; it is skipped (exit status 77) when there is no display, in CI unless
*NATRON_GUI_TESTS=1* is set, or when the plug-ins are missing.

To catch crashes, run Natron under a debugger: with ``--debugger``, the smoke test runs Natron
through ``Tests/gui/natron-lldb.sh``, which runs it under lldb in batch mode, prints the
backtrace of all threads on a crash and exits with status 139 (on macOS, this also avoids the
system "quit unexpectedly" dialog). When it receives TERM, INT or HUP (e.g. on a timeout), it
kills Natron too, which lldb runs in its own process group. The environment variable *NATRON_TEST_WRAPPER* can give
another command to run Natron with. ``natron-lldb.sh`` also works for *NatronRenderer*,
*natron-python* and the *Tests* binary::

    Tests/gui/natron-lldb.sh -o backtrace.txt build/Renderer/NatronRenderer -t script.py

Tips
----

- Call :meth:`saveScreenshot<NatronGui.GuiApp.saveScreenshot>` from the main thread: from the
  script, a timer or a GUI callback. In render threads (e.g. the *afterFrameRender* callback)
  it does nothing and returns False.
- :meth:`natron.quit()<NatronEngine.PyCoreApplication.quit>` removes the auto-saves of the
  projects, and it waits until Natron has finished loading, no nested event loop of Natron (e.g. a
  modal dialog) is running and no project is being saved. Make it the last call of the callback:
  the projects may be closed as soon as the event loop runs, e.g. during a loop that calls PySide's
  ``QCoreApplication.processEvents()``, which Natron doesn't know about. For the same reason, a
  request to close the main window during such a loop is not deferred: wait with timers
  (``QTimer.singleShot()``), as in the example above, rather than with such loops.
- In the GUI, the output of the script (``print``) goes to the Script Editor. To see it in the
  terminal, write to ``sys.__stderr__``.
- With some OpenFX plug-ins, Natron may crash at exit, after the projects were closed, which
  changes the exit status (see `issue #1253 <https://github.com/NatronGitHub/Natron/issues/1253>`_).
  To be safe, also write the result of the test to a file before calling
  :meth:`natron.quit()<NatronEngine.PyCoreApplication.quit>`.
- :meth:`natron.quit()<NatronEngine.PyCoreApplication.quit>` also works with
  *NatronRenderer*, e.g. to make a rendering script exit with an error status.
