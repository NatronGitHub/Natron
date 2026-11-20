.. currentmodule:: NatronEngine
.. _PyCoreApplication:

PyCoreApplication
*****************

**Inherited by:** :doc:`../NatronGui/PyGuiApplication`

Synopsis
--------

This object represents a background instance of Natron.
See :ref:`detailed description<coreApp.details>`...

Functions
^^^^^^^^^

- def :meth:`appendToNatronPath<NatronEngine.PyCoreApplication.appendToNatronPath>` (path)
- def :meth:`getSettings<NatronEngine.PyCoreApplication.getSettings>` ()
- def :meth:`getBuildNumber<NatronEngine.PyCoreApplication.getBuildNumber>` ()
- def :meth:`getInstance<NatronEngine.PyCoreApplication.getInstance>` (idx)
- def :meth:`getActiveInstance<NatronEngine.PyCoreApplication.getActiveInstance>` ()
- def :meth:`getNatronDevelopmentStatus<NatronEngine.PyCoreApplication.getNatronDevelopmentStatus>` ()
- def :meth:`getNatronPath<NatronEngine.PyCoreApplication.getNatronPath>` ()
- def :meth:`getNatronVersionEncoded<NatronEngine.PyCoreApplication.getNatronVersionEncoded>` ()
- def :meth:`getNatronVersionMajor<NatronEngine.PyCoreApplication.getNatronVersionMajor>` ()
- def :meth:`getNatronVersionMinor<NatronEngine.PyCoreApplication.getNatronVersionMinor>` ()
- def :meth:`getNatronVersionRevision<NatronEngine.PyCoreApplication.getNatronVersionRevision>` ()
- def :meth:`getNatronVersionString<NatronEngine.PyCoreApplication.getNatronVersionString>` ()
- def :meth:`getNumCpus<NatronEngine.PyCoreApplication.getNumCpus>` ()
- def :meth:`getNumInstances<NatronEngine.PyCoreApplication.getNumInstances>` ()
- def :meth:`getPluginIDs<NatronEngine.PyCoreApplication.getPluginIDs>` ()
- def :meth:`getPluginIDs<NatronEngine.PyCoreApplication.getPluginIDs>` (filter)
- def :meth:`isBackground<NatronEngine.PyCoreApplication.isBackground>` ()
- def :meth:`is64Bit<NatronEngine.PyCoreApplication.is64Bit>` ()
- def :meth:`isLinux<NatronEngine.PyCoreApplication.isLinux>` ()
- def :meth:`isMacOSX<NatronEngine.PyCoreApplication.isMacOSX>` ()
- def :meth:`isUnix<NatronEngine.PyCoreApplication.isUnix>` ()
- def :meth:`isWindows<NatronEngine.PyCoreApplication.isWindows>` ()
- def :meth:`quit<NatronEngine.PyCoreApplication.quit>` ([exitCode=0])
- def :meth:`setOnProjectCreatedCallback<NatronEngine.PyCoreApplication.setOnProjectCreatedCallback>` (pythonFunctionName)
- def :meth:`setOnProjectLoadedCallback<NatronEngine.PyCoreApplication.setOnProjectLoadedCallback>` (pythonFunctionName)

.. _coreApp.details:

Detailed Description
--------------------

When running Natron there's a **unique** instance of the :doc:`PyCoreApplication` object.
It holds general information about the process.

Generally, throughout your scripts, you can access this object with the variable *natron*
that Natron pre-declared for you, e.g.:

    natron.getPluginIDs()

.. warning::

    The variable **natron** belongs to the module **NatronEngine**, hence make sure to make the following import::

        from NatronEngine import*

    Otherwise with a regular *import* you can still access **natron** by prepending the module::

        NatronEngine.natron

.. warning::

    The variable stored in the module **NatronEngine** contains a reference to a :doc:`PyCoreApplication`.
    If you need to have the GUI functionalities provided by :doc:`../NatronGui/PyGuiApplication`, you must then use
    the variable **natron** belonging to the module **NatronGui**.
    Hence make sure to make the following import to have access to **natron**::

        from NatronGui import*

    With a regular import you can access it using **NatronGui.natron**.

.. warning::

    Make sure to **not** make the 2 following imports, otherwise the **natron** variable will
    not point to something expected::

        #This you should not do!
        from NatronEngine import *
        from NatronGui import *

        #This is OK
        import NatronEngine
        import NatronGui

        #This can also be done for convenience
        from NatronEngine import NatronEngine.natron as NE
        from NatronGui import NatronGui.natron as NG

This class is used only for background (command-line) runs of Natron, that is when you
launch Natron in the following ways::

    Natron -b ...
    Natron -t
    NatronRenderer

For interactive runs of Natron (with the user interface displayed), the derived class :doc:`../NatronGui/PyGuiApplication` is
used instead, which gives access to more GUI specific functionalities.

You should never need to make a new instance of this object yourself. Note that even if you
did, internally the same object will be used and they will all refer to the same Natron
application.

In GUI mode, a :doc`PyGuiApplication` can have several projects opened. For each project
you can refer to them with pre-declared variables *app1* , *app2*, etc...

In background mode, there would be only 1 project opened, so Natron does the following
assignment for you before calling any scripts:

    app = app1

See :doc:`App` to access different opened projects.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: PyCoreApplication()

Defines a new variable pointing to the same underlying application that the *natron* variable
points to. This is equivalent to calling::

    myVar = natron


.. method:: PyCoreApplication.appendToNatronPath(path)


    :param path: :class:`str`

Adds a new path to the Natron search paths. See :ref:`this section<natronPath>` for a detailed explanation
of Natron search paths.



.. method:: PyCoreApplication.getSettings()


    :rtype: :class:`AppSettings<NatronEngine.AppSettings>`

Returns an object containing all Natron settings. The settings are what can be found in
the preferences of Natron.





.. method:: PyCoreApplication.getBuildNumber()


    :rtype: :class:`int`

Returns the build-number of the current version of Natron. Generally this is used for
release candidates, e.g.:

    Natron v1.0.0-RC1  : build number = 1
    Natron v1.0.0-RC2  : build number = 2
    Natron v1.0.0-RC3  : build number = 3



.. method:: PyCoreApplication.getInstance(idx)


    :param idx: :class:`int`
    :rtype: :class:`App<NatronEngine.App>`

Returns the :doc:`App` instance at the given *idx*. Note that *idx* is 0-based, e.g.:
0 would return what's pointed to by *app1*.


.. method:: PyCoreApplication.getActiveInstance()

    :rtype: :class:`App<NatronEngine.App>`

Returns the :doc:`App` instance corresponding to the last project the user interacted with.


.. method:: PyCoreApplication.getNatronDevelopmentStatus()


    :rtype: :class:`str`

Returns a string describing the development status of Natron. This can be one of the following values:

    * Alpha : Meaning the software has unimplemented functionalities and probably many bugs left
    * Beta : Meaning the software has all features that were planned are implemented but there may be bugs
    * RC : Meaning the software seems in a good shape and should be ready for release unless some last minute show-stoppers are found
    * Release : Meaning the software is ready for production




.. method:: PyCoreApplication.getNatronPath()


    :rtype: :class:`sequence`

Returns a sequence of string with all natron :ref:`search paths<natronPath>`.




.. method:: PyCoreApplication.getNatronVersionEncoded()


    :rtype: :class:`int`

Returns an *int* with the version of Natron encoded so that you can compare versions
of Natron like this::

    if natron.getNatronVersionEncoded() >= 20101:
        ...

In that example, Natron's version would be 2.1.1


.. method:: PyCoreApplication.getNatronVersionMajor()


    :rtype: :class:`int`

Returns the major version of Natron. If the version is 1.0.0, that would return 1.




.. method:: PyCoreApplication.getNatronVersionMinor()


    :rtype: :class:`int`

Get the minor version of Natron. If the version is 1.2.0, that would return 2.




.. method:: PyCoreApplication.getNatronVersionRevision()


    :rtype: :class:`int`

Returns the revision number of the version. If the version is 1.2.3, that would return 3.




.. method:: PyCoreApplication.getNatronVersionString()


    :rtype: :class:`str`

Returns the version of Natron as a string, e.g.: *"1.1.0"*




.. method:: PyCoreApplication.getNumCpus()


    :rtype: :class:`int`

Returns the maximum hardware concurrency of the computer. If the computer has 8
hyper-threaded cores, that would return 16.




.. method:: PyCoreApplication.getNumInstances()


    :rtype: :class:`int`

Returns the number of :doc`App` instances currently active.




.. method:: PyCoreApplication.getPluginIDs()


    :rtype: :class:`sequence`

Returns a sequence of strings with all plugin-IDs currently loaded.


.. method:: PyCoreApplication.getPluginIDs(filter)
    :noindex:


    :param filter: :class:`str`
    :rtype: :class:`sequence`

Same as :func:`getPluginIDs()<NatronEngine.PyCoreApplication.getPluginIDs>` but returns
only plug-ins *containing* the given *filter*. Comparison is done **without** case-sensitivity.


.. method:: PyCoreApplication.isBackground()


    :rtype: :class:`bool`

Returns True if Natron is executed in background mode, i.e: from the command-line, without
any graphical user interface displayed.




.. method:: PyCoreApplication.is64Bit()


    :rtype: :class:`bool`

Returns True if Natron is executed on a 64 bit computer.




.. method:: PyCoreApplication.isLinux()


    :rtype: :class:`bool`

Returns True if Natron is executed on a Linux or FreeBSD distribution.




.. method:: PyCoreApplication.isMacOSX()


    :rtype: :class:`bool`

Returns True if Natron is executed on MacOSX.




.. method:: PyCoreApplication.isUnix()


    :rtype: :class:`bool`

Returns True if Natron is executed on Unix. Basically this is equivalent to::

    if natron.isLinux() or natron.isMacOSX():




.. method:: PyCoreApplication.isWindows()


    :rtype: :class:`bool`


Returns True if Natron is executed on Windows.



.. method:: PyCoreApplication.setOnProjectCreatedCallback(pythonFunctionName)

    :param: :class:`str`

Convenience function to set the After Project Created callback. Note that this will override
any callback set in the Preferences-->Python-->After Project created.
This is exactly the same as calling::

    NatronEngine.settings.afterProjectCreated.set(pythonFunctionName)

.. note::

    Clever use of this function can be made in the **init.py** script to do generic stuff
    for all projects (whether they are new projects or loaded projects). For instance
    one might want to add a list of Formats to the project. See the example :ref:`here<startupScripts>`

.. method:: PyCoreApplication.quit([exitCode=0])

    :param exitCode: :class:`int`

Quits Natron, and makes the process exit with the status *exitCode*.
The projects are closed **without** asking to save them, and the renders in progress are
stopped. This is meant for scripts and tests, see :ref:`guiTesting`.

- In the GUI (*Natron*), the projects are closed at the end of the current event-loop
  iteration, so it is safe to call it from a callback or from a timer. The function returns
  immediately and the rest of the calling script is executed: make it the last call of the
  callback. If Natron is still loading (e.g. :func:`quit` is called by the script given on the
  command line), a nested event loop of Natron is running (e.g. a modal dialog), or a project is
  being saved, the projects are closed once it is done. A loop that the script runs itself
  (PySide's ``QCoreApplication.processEvents()``) is not protected: the projects may be closed
  during it, so don't call :func:`quit` before such a loop. The last auto-save of each project is
  removed, as when answering *Discard* to "Save changes?", so that the next launch does not offer
  to restore it. If :func:`quit` is called more than once, the last *exitCode* is the exit
  status.
- In background mode (*NatronRenderer*, or *Natron -b*), the rest of the script is executed,
  then the renders that were not started yet (the Write nodes given with *-w* or *-o*) are
  skipped, and *NatronRenderer* exits with *exitCode*. Called from a render callback (e.g.
  *afterFrameRender*), it stops the render in progress.
- In interpreter mode (*-t*): when called from the script given on the command line, the
  interactive interpreter is not started; when called from the interactive interpreter, this
  raises :class:`SystemExit` (*exitCode*), as :func:`sys.exit` does. When called from a callback
  run by Natron while the interactive interpreter runs, the callback stops, and the interpreter
  exits once it returns to the prompt.
- *natron-python* is a plain Python interpreter where :doc:`PyCoreApplication` does not exist:
  use :func:`sys.exit` there.

For example, to run a check in the GUI and report its result in the exit status::

    Natron --no-dialogs check.py; echo $?

with *check.py*::

    from NatronGui import natron
    from PySide2 import QtCore  # or PySide6

    def check():
        app = natron.getGuiInstance(0)
        ok = app.getViewer("Viewer1") is not None
        natron.quit(0 if ok else 1)

    # wait until the main window is shown and the event loop is running
    QtCore.QTimer.singleShot(500, check)

.. note::

    With some OpenFX plug-ins, Natron may crash at exit, after closing the projects, which
    changes the exit status to that of the crash (e.g. 139 for a segmentation fault): see
    `issue #1253 <https://github.com/NatronGitHub/Natron/issues/1253>`_.
    A test can also write its result to a file before calling :func:`quit`.

.. method:: PyCoreApplication.setOnProjectLoadedCallback(pythonFunctionName)

    :param: :class:`str`

Convenience function to set the Default After Project Loaded callback. Note that this will override
any callback set in the Preferences-->Python-->Default After Project Loaded.
This is exactly the same as calling::

    NatronEngine.settings.defOnProjectLoaded.set(pythonFunctionName)

