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
*    def :meth:`informationDialog<NatronGui.PyGuiApplication.informationDialog>` (title,message)
*    def :meth:`warningDialog<NatronGui.PyGuiApplication.warningDialog>` (title,message)
*    def :meth:`errorDialog<NatronGui.PyGuiApplication.errorDialog>` (title,message)
*    def :meth:`questionDialog<NatronGui.PyGuiApplication.questionDialog>` (title,question)


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



.. method:: NatronGui.PyGuiApplication.informationDialog(title,message)

	:param title: :class:`str`
	:param message: :class:`str`	
	
Shows a modal information dialog to the user with the given window *title* and 
containing the given *message*.

.. method:: NatronGui.PyGuiApplication.warningDialog(title,message)

	:param title: :class:`str`
	:param message: :class:`str`	
	
Shows a modal warning dialog to the user with the given window *title* and 
containing the given *message*.

.. method:: NatronGui.PyGuiApplication.errorDialog(title,message)

	:param title: :class:`str`
	:param message: :class:`str`	
	
Shows a modal error dialog to the user with the given window *title* and 
containing the given *message*.

.. method:: NatronGui.PyGuiApplication.questionDialog(title,message)

	:param title: :class:`str`
	:param message: :class:`str`	
	:rtype: :class:`NatronEngine.StandardButtonEnum`
	
Shows a modal question dialog to the user with the given window *title* and 
containing the given *message*.
The dialog will be a "Yes" "No" dialog, and you can compare the result to the :class:`NatronEngine.StandardButtonEnum` members.