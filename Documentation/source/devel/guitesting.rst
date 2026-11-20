.. _guiTesting:

Testing Natron's GUI from scripts
=================================

A Python script can drive the graphical interface of Natron and check the result, e.g. to
write a regression test or to make screenshots for the documentation. These features make this
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
  the other OpenGL widgets. Only the widgets of Natron are captured, never the screen;
- the test runner, :option:`--test`, and its test-only module ``NatronGuiTest`` (synthetic mouse and
  key events in the viewer and the node graph, a wait for the renders, dialog answers):
  see :ref:`guiTests`.

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

.. _guiTests:

Automated GUI tests (Natron --test)
-----------------------------------

A GUI test is a Python script that drives the Natron user interface the way a user would (key
presses, mouse drags in the viewer and the node graph, dialog answers) and checks the result. Natron
runs it with :option:`--test`, then quits with a status that tells whether the test passed:

.. code-block:: sh

    Natron --test Tests/gui/test_smoke.py

The in-tree tests are in ``Tests/gui/`` (``test_*.py``, with the helpers of ``natrontest.py``).
CMake registers each one as a ``ctest`` test named ``gui.<name>`` with the ``gui`` label:
``ctest -L gui`` runs only the GUI tests, and ``ctest -LE gui`` all the other tests. They need a
display and OpenGL: without them (e.g. on Linux without ``DISPLAY``: run them under ``xvfb-run``),
they are reported as skipped. In CI (``CI`` set), they only run where the job sets
``NATRON_GUI_TESTS=1``, and are skipped otherwise: the Linux job of ``.github/workflows/ci.yml``
sets it, and runs them under ``xvfb-run`` (they are skipped if there is no OpenGL context). Configure with ``-DNATRON_BUILD_GUI_TESTS=OFF`` (a CMake option, unlike the environment variable ``NATRON_GUI_TESTS``) not to
register them. Their caches go to the build tree (``NATRON_DISK_CACHE_PATH``). The plug-ins of ``Tests/OfxTestPlugin`` are on their ``OFX_PLUGIN_PATH``,
before the user's one (with CMake older than 3.22, before the ``OFX_PLUGIN_PATH`` of the configure
step); a test that needs another plug-in (e.g. openfx-misc's Transform) is skipped
when that plug-in is not found.

Test mode
~~~~~~~~~

``Natron --test script.py`` runs the script once the user interface is shown and the project is
created (or loaded, if a project file is also given on the command line), then quits. The exit
status is 0 if the script returns normally, 77 if it is skipped (:func:`NatronGuiTest.skip`, no
display or no OpenGL), and 1 if it raises an exception (e.g. a failed ``assert``), if Natron fails to
start, if a dialog or menu was unexpected, or if the test runs longer than ``NATRON_TEST_TIMEOUT``
seconds (120 by default: the test is then killed, after printing the Python tracebacks of all
threads). A crash ends Natron with the signal (ctest reports it as a failure). A
``sys.exit(status)`` or :meth:`natron.quit(status)<NatronEngine.PyCoreApplication.quit>` in the
script sets the exit status. The script still runs to its end after ``natron.quit()``, and if it
fails afterwards, the test fails. The Python output is not buffered (``PYTHONUNBUFFERED=1``).

The test mode implies :option:`--no-settings` (and the default shortcuts), and sets
``NATRON_TEST_NO_FOCUS=1``, so that the test never takes the keyboard focus or activates a window. It does **not** imply
:option:`--no-dialogs`, which hides parts of the user interface: the tests run in a clean environment
(e.g. a valid OpenColorIO configuration, so that there is no startup dialog), and the dialogs are shown
and work as usual. The test mode also isolates the test from the user's session:

- auto-saves are neither written nor restored, and the "An auto-saved project was found" question
  is never asked (the user's auto-saves are left untouched);
- Natron does not check for updates, and does not ask about the default appearance, the shortcuts or
  the OpenColorIO configuration (set ``OCIO``);
- a test that expects a modal dialog or a menu gives its answer beforehand with
  :func:`NatronGuiTest.setDialogAnswers`: when the dialog is shown, its button is clicked (or the menu
  item triggered), so that the real code runs. A modal dialog or menu that gets no answer is
  **unexpected**: it is printed, closed so that it does not block, and the test fails, even if it is
  skipped afterwards;
- the settings are not written to the user's preferences: the Natron settings, the plug-in
  settings (which are not read either: the plug-ins keep their defaults), the paths added with
  ``natron.appendToNatronPath()``, the shortcuts, the window geometry, the file dialog state, the Tab dialog's plug-in ranking, the colour palette and the
  cache version are not saved, and the other settings (e.g. the recent files) go to a temporary
  directory of the run, removed when Natron exits;
- the dialogs are recorded (:func:`NatronGuiTest.getDialogLog`).

The test mode does not isolate everything:

- a test run by hand (not through ``ctest``) uses the user's disk cache and OpenFX plug-in cache,
  unless ``NATRON_DISK_CACHE_PATH`` is set (``ctest`` sets it to the build tree);
- a test that opens a project file, and is ended by the watchdog or a crash, leaves that project's
  ``.lock`` file: give the tests copies of the projects;
- ``NATRON_TEST_NO_FOCUS=0`` (set explicitly) lets the windows take the focus;
- debug builds do not trap floating point exceptions in test mode: the OpenGL driver's threads (e.g.
  Mesa llvmpipe's, under Xvfb) inherit the trapping, and their shaders raise exceptions.

A test script runs from the event loop, once the main window is shown: it needs no timer, and
Natron quits when the script returns, as with
:meth:`natron.quit(status)<NatronEngine.PyCoreApplication.quit>`.

The output of the script (``print()``, tracebacks) goes to the terminal, not to the Script Editor.

Writing a test
~~~~~~~~~~~~~~

The script runs with the variable ``app`` set to the application instance (as ``app1``), and its
directory on ``sys.path``. A test is a sequence of user actions, each followed by a wait for the
renders and events it triggers, then checks:

.. code-block:: python

    import NatronGuiTest
    from natrontest import *          # Tests/gui/natrontest.py

    require_plugin(GENERATOR)         # skips the test if the plug-in is missing

    gen = app.createNode(GENERATOR)
    app.getNode("Viewer1").connectInput(0, gen)
    NatronGuiTest.sendKey("Viewer1", "F")   # fit the image in the viewer
    assert NatronGuiTest.waitIdle()

    app.selectNode(gen, True)
    NatronGuiTest.sendKey("NodeGraph", "Backspace")   # delete, as the user does
    NatronGuiTest.sendKey("NodeGraph", "Ctrl+Z")      # and undo
    assert NatronGuiTest.waitIdle()
    assert app.getNode(gen.getScriptName()) is not None

Never use fixed delays: :func:`NatronGuiTest.waitIdle` returns as soon as the renders are done and
their results are displayed.

The NatronGuiTest module
~~~~~~~~~~~~~~~~~~~~~~~~

``NatronGuiTest`` is a test-only module: it exists only in Natron's test mode (:option:`--test`) and is
not part of the Natron Python API, which keeps its backward compatibility commitment. It sends
synthetic Qt events to the widgets; the real mouse cursor and the keyboard are not used.

.. module:: NatronGuiTest

.. function:: waitIdle(timeout=30.0, settle=0.2)

   Process the events until no render (viewer, Write, node preview) has been running for *settle*
   seconds, so that the results of the renders are displayed. Return ``True``, or ``False`` if this
   did not happen within *timeout* seconds. The Python lock is released while waiting, so that
   the renders can run Python expressions.

.. function:: isRendering()

   Return ``True`` if a render is running or queued.

.. function:: penEvent(target, type, x, y, button='left', modifiers='')

   Send a mouse event to *target*: a viewer (its node script name, e.g. ``"Viewer1"``), at the
   canvas coordinates (*x*, *y*) (the coordinates of the image and of the overlays); the node graph
   of the project (``"nodeGraph"``, its object name, or ``"NodeGraph"``), or another node graph by its
   object name, at the node graph coordinates (*x*, *y*) (as returned by
   :func:`Effect.getPosition()<NatronEngine.Effect.getPosition>`); or any other widget by its object
   name, at its widget coordinates. *type* is
   ``"press"``, ``"move"`` or ``"release"``; *button* is ``"left"``, ``"middle"``, ``"right"`` or
   ``"none"``; *modifiers* is a ``+``-separated list of ``Shift``, ``Ctrl``, ``Alt`` and ``Meta``
   (``Ctrl`` is the Command key on macOS, as in Natron's shortcuts). A move has the buttons pressed by
   the previous press events. The widget gets *Enter* and *Leave* events as the pointer moves from a
   widget to another. A press, and a key sent with :func:`sendKey`, first send *FocusOut* to the previous target and
   *FocusIn* to the new one, as a click or typing would: the window is not activated (no focus is
   taken from the user), but the widgets react as usual, e.g. the node graph becomes the last focused
   graph, which pasting and rendering with *F7* use.

.. function:: drag(target, x0, y0, x1, y1, steps=10, button='left', modifiers='')

   Drag from (*x0*, *y0*) to (*x1*, *y1*) in *target* (see :func:`penEvent`): a move, a press,
   *steps* moves, and a release.

.. function:: sendKey(target, keys)

   Send *keys* to *target* (a viewer node script name, the node graph, or a widget by its object name,
   as for :func:`penEvent`). *keys* uses the portable
   text of ``QKeySequence``, e.g. ``"F"``, ``"Ctrl+Z"``, ``"Backspace"``, ``"Return"``, or several keys
   separated by commas. As for a real key press, a key that the target does not handle itself
   triggers the matching shortcut of the window's menus, e.g. *Edit > Undo*.

.. function:: getDisplayedPixel(viewer, x, y)

   Return the 8-bit ``(r, g, b, a)`` colour that the viewer displays at the canvas coordinates
   (*x*, *y*), after the display LUT.

.. function:: setDialogAnswers(answers)

   Set the answers of the next modal dialogs and menus (popups), used in order. Each answer is:

   - a button name (``"yes"``, ``"no"``, ``"ok"``, ``"cancel"``, ``"save"``, ``"discard"``,
     ``"close"``, …): clicks that button of the dialog;
   - for a menu, the text of the item to trigger (without ``&``), in the menu or, if not found there, in
     its submenus (breadth first: the first match wins), or its path, the item texts separated by ``/``
     (e.g. ``"Other/Dot"``): it is selected as the user does (the item is made active and *Return* is pressed,
     which opens the submenus on the way), so that the code that runs the menu gets that item;
   - a callable, called with ``(title, text, className)`` while the window is shown: it returns one of
     the above, or ``None`` if it handled the window itself (e.g. a file dialog, through PySide's
     ``QApplication.activeModalWidget()``).

   A dialog or menu shown when the list is empty, or without such a button or item, is unexpected: it
   is closed and the test fails. Every popup is watched: a popup that is not a menu (e.g. the list of a
   Qt combo box or of a completer) can only be answered with a callable. A callable that returns
   ``None`` without closing a modal dialog leaves the test blocked until the watchdog ends it.

.. function:: getDialogLog()

   Return the modal dialogs and menus shown so far, as a list of dictionaries with the keys ``type``
   (``"dialog"`` or ``"menu"`` when answered, ``"unexpected"`` when closed), ``title``, ``text`` and
   ``answer`` (the button name or menu item, ``"handler"`` when a callable handled it, or
   ``"reject"``).

.. function:: clearDialogLog()

   Clear the list returned by :func:`getDialogLog`.

.. function:: requireInteractive(reason='needs the real focus or mouse')

   Skip the test (exit status 77) unless the environment variable ``NATRON_TEST_INTERACTIVE`` is ``1``.
   A test that needs the real keyboard focus or mouse (which the synthetic events of this module do not
   use) calls it first, and has the line ``# NatronGuiTest: interactive``, which gives it the ctest label
   ``gui-interactive`` (in addition to ``gui``). They are meant for a display that no user works on, e.g. a
   CI job under Xvfb that sets the variable, or a virtual machine; on a desktop, they only run when the
   user opts in, because they take the keyboard and mouse. The Linux job of ``.github/workflows/ci.yml``
   sets ``NATRON_TEST_INTERACTIVE=1``.

.. function:: skip(reason)

   Stop the test and report it as skipped (exit status 77), e.g. when a plug-in it needs is missing.
   It raises the ``NatronGuiTest.Skip`` exception.
