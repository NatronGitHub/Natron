.. module:: NatronGui
.. _GuiApp:

PyModalDialog
******************

**Inherits** :doc:`UserParamHolder` :class:`QDialog`


Synopsis
-------------

A modal dialog to ask informations to the user or to warn about something.
See :ref:`detailed<modalDialog.details>` description...


Functions
^^^^^^^^^

*    def :meth:`addWidget<NatronGui.PyModalDialog.addWidget>` (widget)
*    def :meth:`getParam<NatronGui.PyModalDialog.getParam>` (scriptName)
*    def :meth:`insertWidget<NatronGui.PyModalDialog.insertWidget>` (index,widget)
*    def :meth:`setParamChangedCallback<NatronGui.PyModalDialog.setParamChangedCallback>` (callback)

.. _modalDialog.details:

Detailed Description
---------------------------

The modal dialog is a way to ask the user for data or to inform him/her about something going on.
A modal window means that control will not be returned to the user (i.e no event will be processed) until
the user closed the dialog.

If you are looking for a simple way to just ask a question or report an error, warning or even
just a miscenalleous information, use the :func:`informationDialog(title,message)<NatronGui.PyGuiApplication.informationDialog>` function.

To create a new :doc:`PyModalDialog`, just use the :func:`createModalDialog()<NatronGui.GuiApp.createModalDialog>` function, e.g::

	# In the Script Editor
	
	dialog = app1.createModalDialog()
	
To show the dialog to the user, use the :func:`exec()<>` function inherited from :class:`QDialog` ::

	dialog.exec()
	
Note that once :func:`exec()<>` is called, no instruction will be executed until the user closed the dialog. 

The modal dialog always has *OK* and *Cancel* buttons. To query which button the user pressed, inspect the return value of the :func:`exec()<>` call::

	if dialog.exec():
		#The user pressed OK
		...
	else:
		#The user pressed Cancel or Escape
		
Adding user parameters:
^^^^^^^^^^^^^^^^^^^^^^^


You can start adding user parameters using all the :func:`createXParam<>` functions inherited from :doc:`UserParamHolder` class.

Once all your parameters are created, create the GUI for them using the :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>` function::

	myInteger = dialog.createIntParam("myInt","This is an integer very important")
	myInteger.setAnimationEnabled(False)
	myInteger.setAddNewLine(False)
	
	#Create a boolean on the same line
	myBoolean = dialog.createBooleanParam("myBool","Yet another important boolean")
	
	dialog.refreshUserParamsGUI()
	
	dialog.exec()
	
You can then retrieve the value of a parameter once the dialog is finished using the :func:`getParam(scriptName)<NatronGui.PyModalDialog.getParam>` function::

	if dialog.exec():
		intValue = dialog.getParam("myInt").get()
		boolValue = dialog.getParam("myBool").get()

		
Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronGui.PyModalDialog.addWidget(widget)

	:param widget: :class:`PySide.QtGui.QWidget`
	
Append a QWidget inherited *widget* at the bottom of the dialog. This allows to add custom GUI created directly using PySide.




.. method:: NatronGui.PyModalDialog.getParam(scriptName)
	
	:param scriptName: :class:`str`
	:rtype: :class:`Param<NatronEngine.Param>`
	
Returns the user parameter with the given *scriptName* if it exists or *None* otherwise.




.. method:: NatronGui.PyModalDialog.insertWidget(index,widget)

	:param index: :class:`int`
	:param widget: :class:`PySide.QtGui.QWidget`
	
Inserts a QWidget inherited *widget* at the given *index* of the layout in the dialog. This allows to add custom GUI created directly using PySide.




.. method:: NatronGui.PyModalDialog.setParamChangedCallback(callback)
	
	:param callback: :class:`str`

Registers the given Python *callback* to be called whenever a user parameter changed. 
The *callback* should be the name of a Python defined function (taking no parameter). 

The variable *thisParam* will be declared upon calling the callback, referencing the parameter that just changed.
Example::

	def myCallback():
		if thisParam.getScriptName() == "myInt":
			intValue = thisParam.get()
			if intValue > 0:
				myBoolean.setVisible(False)
		
	dialog.setParamChangedCallback("myCallback")
		


