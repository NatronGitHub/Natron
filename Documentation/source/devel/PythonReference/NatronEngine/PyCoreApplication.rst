.. module:: NatronEngine
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


.. method:: NatronEngine.PyCoreApplication.appendToNatronPath(path)


    :param path: :class:`str<NatronEngine.std::string>`

Adds a new path to the Natron search paths. See :ref:`this section<natronPath>` for a detailed explanation
of Natron search paths.



.. method:: NatronEngine.PyCoreApplication.getSettings()


    :rtype: :class:`AppSettings<NatronEngine.AppSettings>`

Returns an object containing all Natron settings. The settings are what can be found in
the preferences of Natron.





.. method:: NatronEngine.PyCoreApplication.getBuildNumber()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the build-number of the current version of Natron. Generally this is used for
release candidates, e.g.:

    Natron v1.0.0-RC1  : build number = 1
    Natron v1.0.0-RC2  : build number = 2
    Natron v1.0.0-RC3  : build number = 3



.. method:: NatronEngine.PyCoreApplication.getInstance(idx)


    :param idx: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`App<NatronEngine.App>`

Returns the :doc:`App` instance at the given *idx*. Note that *idx* is 0-based, e.g.:
0 would return what's pointed to by *app1*.


.. method:: NatronEngine.PyCoreApplication.getActiveInstance()

    :rtype: :class:`App<NatronEngine.App>`

Returns the :doc:`App` instance corresponding to the last project the user interacted with.


.. method:: NatronEngine.PyCoreApplication.getNatronDevelopmentStatus()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns a string describing the development status of Natron. This can be one of the following values:

    * Alpha : Meaning the software has unimplemented functionalities and probably many bugs left
    * Beta : Meaning the software has all features that were planned are implemented but there may be bugs
    * RC : Meaning the software seems in a good shape and should be ready for release unless some last minute show-stoppers are found
    * Release : Meaning the software is ready for production




.. method:: NatronEngine.PyCoreApplication.getNatronPath()


    :rtype: :class:`sequence`

Returns a sequence of string with all natron :ref:`search paths<natronPath>`.




.. method:: NatronEngine.PyCoreApplication.getNatronVersionEncoded()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns an *int* with the version of Natron encoded so that you can compare versions
of Natron like this::

    if natron.getNatronVersionEncoded() >= 20101:
        ...

In that example, Natron's version would be 2.1.1


.. method:: NatronEngine.PyCoreApplication.getNatronVersionMajor()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the major version of Natron. If the version is 1.0.0, that would return 1.




.. method:: NatronEngine.PyCoreApplication.getNatronVersionMinor()


    :rtype: :class:`int<PySide.QtCore.int>`

Get the minor version of Natron. If the version is 1.2.0, that would return 2.




.. method:: NatronEngine.PyCoreApplication.getNatronVersionRevision()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the revision number of the version. If the version is 1.2.3, that would return 3.




.. method:: NatronEngine.PyCoreApplication.getNatronVersionString()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the version of Natron as a string, e.g.: *"1.1.0"*




.. method:: NatronEngine.PyCoreApplication.getNumCpus()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the maximum hardware concurrency of the computer. If the computer has 8
hyper-threaded cores, that would return 16.




.. method:: NatronEngine.PyCoreApplication.getNumInstances()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of :doc`App` instances currently active.




.. method:: NatronEngine.PyCoreApplication.getPluginIDs()


    :rtype: :class:`sequence`

Returns a sequence of strings with all plugin-IDs currently loaded.


.. method:: NatronEngine.PyCoreApplication.getPluginIDs(filter)


    :param filter: :class:`str`
    :rtype: :class:`sequence`

Same as :func:`getPluginIDs()<NatronEngine.PyCoreApplication.getPluginIDs>` but returns
only plug-ins *containing* the given *filter*. Comparison is done **without** case-sensitivity.


.. method:: NatronEngine.PyCoreApplication.isBackground()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if Natron is executed in background mode, i.e: from the command-line, without
any graphical user interface displayed.




.. method:: NatronEngine.PyCoreApplication.is64Bit()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if Natron is executed on a 64 bit computer.




.. method:: NatronEngine.PyCoreApplication.isLinux()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if Natron is executed on a Linux or FreeBSD distribution.




.. method:: NatronEngine.PyCoreApplication.isMacOSX()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if Natron is executed on MacOSX.




.. method:: NatronEngine.PyCoreApplication.isUnix()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if Natron is executed on Unix. Basically this is equivalent to::

    if natron.isLinux() or natron.isMacOSX():




.. method:: NatronEngine.PyCoreApplication.isWindows()


    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns True if Natron is executed on Windows.



.. method:: NatronEngine.PyCoreApplication.setOnProjectCreatedCallback(pythonFunctionName)

    :param: :class:`str<NatronEngine.std::string>`

Convenience function to set the After Project Created callback. Note that this will override
any callback set in the Preferences-->Python-->After Project created.
This is exactly the same as calling::

    NatronEngine.settings.afterProjectCreated.set(pythonFunctionName)

.. note::

    Clever use of this function can be made in the **init.py** script to do generic stuff
    for all projects (whether they are new projects or loaded projects). For instance
    one might want to add a list of Formats to the project. See the example :ref:`here<startupScripts>`

.. method:: NatronEngine.PyCoreApplication.setOnProjectLoadedCallback(pythonFunctionName)

    :param: :class:`str<NatronEngine.std::string>`

Convenience function to set the Default After Project Loaded callback. Note that this will override
any callback set in the Preferences-->Python-->Default After Project Loaded.
This is exactly the same as calling::

    NatronEngine.settings.defOnProjectLoaded.set(pythonFunctionName)

