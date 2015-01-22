.. module:: NatronGui
.. _PyGuiApplication:

PyGuiApplication
****************

**Inherits** :doc:`PyCoreApplication`

Synopsis
--------

See :doc:`PyCoreApplication` for a detailed explanation of the purpose of this object.
This class is only used when Natron is run in GUI mode (with user interface). 
It gives you access to more GUI functionalities via the :doc:`GuiApp` class.

Functions
^^^^^^^^^


*    def :meth:`getGuiInstance<NatronGui.PyGuiApplication.getGuiInstance>` (idx)


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: PyGuiApplication()

See :func:`PyCoreApplication()<NatronEngine.PyCoreApplication.PyCoreApplication>`


.. method:: NatronGui.PyGuiApplication.getGuiInstance(idx)


    :param idx: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`GuiApp<NatronGui.GuiApp>`

Same as :func:`getInstance(idx)<NatronEngine.PyCoreApplication.getInstance>` but returns
instead an instance of a GUI project.

Basically you should never call this function as Natron pre-declares all opened projects
with the following variables: *app1* for the first opened project, *app2* for the second, and so on...







