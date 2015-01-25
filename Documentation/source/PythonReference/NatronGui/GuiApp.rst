.. module:: NatronGui
.. _GuiApp:

GuiApp
*********

**Inherits** :doc:`App`


Synopsis
-------------

This class is used for GUI application instances.
See :ref:`detailed<guiApp.details>` description...


Functions
^^^^^^^^^

*    def :meth:`createModalDialog<NatronGui.GuiApp.createModalDialog>` ()


.. _guiApp.details:

Detailed Description
---------------------------

See :doc:`App` for the documentation of most functionnalities of this class.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronGui.GuiApp.createModalDialog()

    :rtype: :class:`PyModalDialog<NatronGui.PyModalDialog>`

Creates a :doc:`modal dialog<NatronGui.PyModalDialog>` : the control will not be returned to the user until the dialog is not closed.
Once the dialog is created, you can enrich it with :doc:`parameters<NatronEngine.Param>` or even
raw PySide Qt widgets.
To show the dialog call the function :func:`exec()<>` on the dialog.