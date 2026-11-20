.. _debuggingPython:

Debugging Python scripts
========================

This page explains where Natron shows the output and the errors of your Python code, and how
to test scripts outside of the user interface.

The Script Editor
-----------------

The *Script Editor* panel has an input area, where you type Python code, and an output area.
The buttons above them, and their default shortcuts, are:

- *Previous Script* and *Next Script* (:kbd:`Ctrl+[` and :kbd:`Ctrl+]`): go through the history
  of the scripts you executed. *Clear History* empties it.
- *Open and execute a script*, *Open a script without executing it*, *Save the current script*.
- *Execute the current script* (:kbd:`Ctrl+Return`).
- *Show/Hide the output area*, and *Clear the output area* (:kbd:`Ctrl+Backspace`).
- the last button, when checked, makes the output area also show, in gray, the
  variables that Natron declares for you (``app1.Blur1`` ...) when nodes or parameters are
  created. This is the ``printAutoDeclaredVars`` setting of the Preferences.

On macOS, :kbd:`Ctrl` is the :kbd:`Cmd` key. The shortcuts can be changed in the shortcut editor.

What you ``print()``, and the exceptions raised by your code, appear in the output area. So do
the output and the errors of the Python code that Natron runs for you in GUI mode: callbacks,
menu commands, and scripts loaded from the command line.
In the Script Editor, use ``app1``, ``app2``... to refer to a project, not ``app``
(see :ref:`natronObjects`).

Messages at startup
-------------------

Startup scripts and PyPlugs are loaded before any Script Editor exists, so their messages go
elsewhere:

- in GUI mode, Natron prints a line for each startup script it runs, e.g.
  ``Info: init.py script found and loaded at /home/me/.Natron/init.py``, or
  ``Info: init.py script not loaded (this is not an error)``. Start Natron from a terminal to see them;
- an exception in ``init.py`` or ``initGui.py`` is printed to the terminal and written to the
  error log, :menuselection:`Display --> Show Project Errors Log...`;
- a ``.py`` file that looks like a PyPlug but fails to import writes
  *Was not recognized as a PyPlug* and the Python error to the error log;
- what a startup script prints is shown in the terminal, and in the Script Editor of the first
  project if one exists.

If a PyPlug doesn't appear in the menus, check, in this order: that it is in a
:ref:`search path<natronPath>`; that it has a line starting with ``# Natron PyPlug``; that it
defines ``getLabel()`` and ``createInstance(app, group)`` (without them the file is ignored
without any message); that it doesn't import NatronGui if you run NatronRenderer; and the error
log. See :ref:`pyplugWriting`.

Callback errors
---------------

Natron checks the names of the arguments of each callback before calling it. If they don't
match, it doesn't call the function and prints, in the Script Editor, the signature that it
expects, e.g.::

    Wrong signature of onParamChanged callback: The param changed callback supports the following signature(s):
    - callback(thisParam, thisNode, thisGroup, app, userEdited)

The signatures are listed in :ref:`callbacks`. A callback defined in a PyPlug or in a module
must be registered with its module name, e.g. ``MyPyPlug.myCallback``.

Testing scripts without the user interface
------------------------------------------

The interpreter mode runs a script, then gives you a Python prompt in the terminal, with the
Natron modules loaded and ``app`` defined::

    NatronRenderer -t /path/to/myScript.py

You can then inspect the nodes the script created, call your functions again, or use the
standard Python debugger (``import pdb; pdb.pm()`` after an exception). Nothing is rendered
until you call :func:`app.render()<NatronEngine.App.render>`. Only ``init.py`` is run in this mode,
not ``initGui.py``. See :ref:`natronExec`.

Checking versions
-----------------

When you report a problem, or when a script must work with several Natron versions, these
lines print the versions in use::

    import sys
    print(sys.version)                      # Python version
    import qtpy
    print(qtpy.API_NAME, qtpy.QT_VERSION)   # Qt binding (PySide2 or PySide6) and Qt version
    print(NatronEngine.natron.getNatronVersionString())

``natron.getNatronVersionMajor()``, ``getNatronVersionMinor()`` and ``getNatronVersionRevision()``
return the parts of the Natron version as integers.
