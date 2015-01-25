.. module:: NatronGui
.. _PyGuiApplication:

PyGuiApplication
****************

**Inherits** :doc:`PyCoreApplication`

Synopsis
--------

This is the unique main object representing the process Natron when started in GUI mode (with user interface).

Functions
^^^^^^^^^


*    def :meth:`getGuiInstance<NatronGui.PyGuiApplication.getGuiInstance>` (idx)


Detailed Description
--------------------




.. class:: PyGuiApplication()



.. method:: NatronGui.PyGuiApplication.getGuiInstance(idx)


    :param idx: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`GuiApp<NatronGui.GuiApp>`

Retrieves the application instance (opened project) at the given *idx*. Note that you should never call it
directly since Natron :ref:`auto-declares<autoVar>` variables referencing application instances itself::

    app1 #References the first opened project
    app2 #References the second opened project, etc...






