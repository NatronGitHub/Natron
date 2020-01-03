.. module:: NatronGui
.. _pyModalDialog:

PyModalDialog
******************

**Inherits** `QDialog <https://pyside.github.io/docs/pyside/PySide/QtGui/QDialog.html>`_ :doc:`../NatronEngine/UserParamHolder`


Synopsis
-------------

A modal dialog to ask information to the user or to warn about something.
See :ref:`detailed<modalDialog.details>` description...


Functions
^^^^^^^^^

- def :meth:`addWidget<NatronGui.PyModalDialog.addWidget>` (widget)
- def :meth:`getParam<NatronGui.PyModalDialog.getParam>` (scriptName)
- def :meth:`insertWidget<NatronGui.PyModalDialog.insertWidget>` (index,widget)
- def :meth:`setParamChangedCallback<NatronGui.PyModalDialog.setParamChangedCallback>` (callback)

.. _modalDialog.details:

Detailed Description
---------------------------

The modal dialog is a way to ask the user for data or to inform him/her about something going on.
A modal window means that control will not be returned to the user (i.e. no event will be processed) until
the user closed the dialog.

If you are looking for a simple way to just ask a question or report an error, warning or even
just a miscenalleous information, use the :func:`informationDialog(title,message)<NatronGui.PyGuiApplication.informationDialog>` function.

To create a new :doc:`PyModalDialog`, just use the :func:`createModalDialog()<NatronGui.GuiApp.createModalDialog>` function, e.g.:

    # In the Script Editor

    dialog = app1.createModalDialog()

To show the dialog to the user, use the :func:`exec_()<>` function inherited from :class:`QDialog` ::

    dialog.exec_()

Note that once :func:`exec_()<>` is called, no instruction will be executed until the user closed the dialog.

The modal dialog always has *OK* and *Cancel* buttons. To query which button the user pressed, inspect the return value of the :func:`exec_()<>` call::

    if dialog.exec_():
        #The user pressed OK
        ...
    else:
        #The user pressed Cancel or Escape

Adding user parameters:
^^^^^^^^^^^^^^^^^^^^^^^

You can start adding user parameters using all the :func:`createXParam<>` functions inherited from the :class:`NatronEngine.UserParamHolder` class.

Once all your parameters are created, create the GUI for them using the :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>` function::

    myInteger = dialog.createIntParam("myInt","This is an integer very important")
    myInteger.setAnimationEnabled(False)
    myInteger.setAddNewLine(False)

    #Create a boolean on the same line
    myBoolean = dialog.createBooleanParam("myBool","Yet another important boolean")

    dialog.refreshUserParamsGUI()

    dialog.exec_()

You can then retrieve the value of a parameter once the dialog is finished using the :func:`getParam(scriptName)<NatronGui.PyModalDialog.getParam>` function::

    if dialog.exec_():
        intValue = dialog.getParam("myInt").get()
        boolValue = dialog.getParam("myBool").get()

.. warning::

    Unlike the :ref:`Effect<Effect>` class, parameters on modal dialogs are not automatically declared by Natron,
    which means you cannot do stuff like *dialog.intValue*



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronGui.PyModalDialog.addWidget(widget)

    :param widget: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`_

Append a QWidget inherited *widget* at the bottom of the dialog. This allows to add custom GUI created directly using PySide
that will be inserted **after** any custom parameter.




.. method:: NatronGui.PyModalDialog.getParam(scriptName)

    :param scriptName: :class:`str`
    :rtype: :class:`Param<NatronEngine.Param>`

Returns the user parameter with the given *scriptName* if it exists or *None* otherwise.




.. method:: NatronGui.PyModalDialog.insertWidget(index,widget)

    :param index: :class:`int`
    :param widget: :class:`PySide.QtGui.QWidget`

Inserts a QWidget inherited *widget* at the given *index* of the layout in the dialog. This allows to add custom GUI created directly using PySide.
The widget will always be inserted **after** any user parameter.




.. method:: NatronGui.PyModalDialog.setParamChangedCallback(callback)

    :param callback: :class:`str`

Registers the given Python :ref:`callback<callbacks>` to be called whenever a user parameter changed.
The parameter *callback* is a string that should contain the name of a Python function.

The signature of the :ref:`callback<callbacks>` used on :ref:`PyModalDialog<pyModalDialog>` is::

    callback(paramName, app, userEdited)

- **paramName** indicating the :ref:`script-name<autoVar>` of the parameter which just had its value changed.
- **app** : This variable will be set so it points to the correct :ref:`application instance<App>`.
- **userEdited** : This indicates whether or not the parameter change is due to user interaction (i.e: because the user changed
  the value by theirself) or due to another parameter changing the value of the parameter
  via a derivative of the :func:`setValue(value)<>` function.

Example::

    def myParamChangedCallback(paramName, app, userEdited):
        if paramName == "myInt":
            intValue = thisParam.get()
            if intValue > 0:
                myBoolean.setVisible(False)

    dialog.setParamChangedCallback("myParamChangedCallback")



