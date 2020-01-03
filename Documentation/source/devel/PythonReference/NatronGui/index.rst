.. module:: NatronGui

.. _NatronGui:

NatronGui
*********

Detailed Description
--------------------

Here are listed all classes being part of NatronEngine module. This module is loaded
by Natron natively in GUI mode only.  In that case, access is granted to these classes
in your scripts without importing anything.
Scripts that want to operate both in command line background mode and in GUI mode should
poll the :func:`isBackground()<NatronEngine.PyCoreApplication.isBackground>` function on the **natron**
object before calling functions dependent on the module :mod:`NatronGui`.
E.g::

    if not NatronEngine.natron.isBackground():
        # do GUI only stuff here


.. toctree::
    :maxdepth: 1

    GuiApp
    PyGuiApplication
    PyModalDialog
    PyPanel
    PyTabWidget
    PyViewer


