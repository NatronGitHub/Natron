.. module:: NatronGui
.. _PyGuiApplication:

PyGuiApplication
****************

**Inherits** :doc:`../NatronEngine/PyCoreApplication`

Synopsis
--------

See :doc:`../NatronEngine/PyCoreApplication` for a detailed explanation of the purpose of this object.
This class is only used when Natron is run in GUI mode (with user interface).
It gives you access to more GUI functionalities via the :doc:`GuiApp` class.

Functions
^^^^^^^^^

- def :meth:`addMenuCommand<NatronGui.PyGuiApplication.addMenuCommand>` (grouping,function)
- def :meth:`addMenuCommand<NatronGui.PyGuiApplication.addMenuCommand>` (grouping,function,key,modifiers)
- def :meth:`getGuiInstance<NatronGui.PyGuiApplication.getGuiInstance>` (idx)
- def :meth:`informationDialog<NatronGui.PyGuiApplication.informationDialog>` (title,message)
- def :meth:`warningDialog<NatronGui.PyGuiApplication.warningDialog>` (title,message)
- def :meth:`errorDialog<NatronGui.PyGuiApplication.errorDialog>` (title,message)
- def :meth:`questionDialog<NatronGui.PyGuiApplication.questionDialog>` (title,question)

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: PyGuiApplication()

See :func:`PyCoreApplication()<NatronEngine.PyCoreApplication.PyCoreApplication>`


.. method:: NatronGui.PyGuiApplication.addMenuCommand(grouping, function)


    :param grouping: :class:`str`
    :param function: :class:`str`

Adds a new menu entry in the menubar of Natron. This should be used **exclusively** in the
*initGui.py* initialisation script.

The *grouping* is a string indicating a specific menu entry where each submenu is separated
from its parent menu with a */*::

    File/Do something special

    MyStudio/Scripts/Our special trick

The *function* is the name of a python defined function.

.. warning::

    If called anywhere but from the *initGui.py* script, this function will fail to
    dynamically add a new menu entry.


Example::

    def printLala():
        print("Lala")

    natron.addMenuCommand("Inria/Scripts/Print lala script","printLala")

This registers in the menu *Inria-->Scripts* an entry named *Print lala script* which will
print *Lala* to the Script Editor when triggered.

.. method:: NatronGui.PyGuiApplication.addMenuCommand(grouping, function, key, modifiers)

    :param grouping: :class:`str`
    :param function: :class:`str`
    :param key: :class:`PySide.QtCore.Qt.Key`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`

Same as :func:`addMenuCommand(grouping,function)<NatronGui.PyGuiApplication.addMenuCommand>`
excepts that it accepts a default shortcut for the action.
See PySide documentation for possible keys and modifiers.

The user will always be able to modify the shortcut from the built-in shortcut editor of Natron anyway.



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
