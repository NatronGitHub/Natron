.. module:: NatronGui
.. _pypanel:

PyPanel
********

**Inherits** :`QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>` :doc:`../NatronEngine/UserParamHolder`


Synopsis
-------------

A custom PySide pane that can be docked into :doc:`PyTabWidget`.
See :ref:`detailed<pypanel.details>` description...


Functions
^^^^^^^^^

- def :meth:`PyPanel<NatronGui.PyPanel.PyPanel>` (scriptName,label,useUserParameters,app)
- def :meth:`addWidget<NatronGui.PyPanel.addWidget>` (widget)
- def :meth:`getPanelLabel<NatronGui.PyPanel.getPanelLabel>` ()
- def :meth:`getPanelScriptName<NatronGui.PyPanel.getPanelScriptName>` ()
- def :meth:`getParam<NatronGui.PyPanel.getParam>` (scriptName)
- def :meth:`getParams<NatronGui.PyPanel.getParams>` ()
- def :meth:`insertWidget<NatronGui.PyPanel.insertWidget>` (index,widget)
- def :meth:`onUserDataChanged<NatronGui.PyPanel.onUserDataChanged>` ()
- def :meth:`setParamChangedCallback<NatronGui.PyPanel.setParamChangedCallback>` (callback)
- def :meth:`save<NatronGui.PyPanel.save>` ()
- def :meth:`setPanelLabel<NatronGui.PyPanel.setPanelLabel>` (label)
- def :meth:`restore<NatronGui.PyPanel.restore>` (data)

.. _pypanel.details:

Detailed Description
---------------------------

The :doc:`PyPanel` class can be used to implement custom PySide widgets that can then be
inserted as tabs into :doc:`tab-widgets<PyTabWidget>` .

There are 2 possible usage of this class:

    * Sub-class it and create your own GUI using `PySide <http://qt-project.org/wiki/PySideDocumentation>`_
    * Use the API proposed by :doc:`PyPanel` to add custom user :doc:`parameters<../NatronEngine/Param>` as done for :doc:`PyModalDialog`.

Sub-classing:
^^^^^^^^^^^^^

When sub-classing the :doc:`PyPanel` class, you should specify when calling the base class
constructor that you do not want to use user parameters, as this might conflict with the
layout that you will use::

    class MyPanel(NatronGui.PyPanel):
        def __init__(scriptName,label,app):
            NatronGui.PyPanel.__init__(scriptName,label,False,app)
            ...

You're then free to use all features proposed by `PySide <http://qt-project.org/wiki/PySideDocumentation>`_
in your class, including `signal/slots <http://qt-project.org/wiki/Signals_and_Slots_in_PySide>`_
See the following :ref:`example <pysideExample>`.


Using the PyPanel API:
^^^^^^^^^^^^^^^^^^^^^^

You can start adding user parameters using all the :func:`createXParam<>` functions inherited from the :doc:`../NatronEngine/UserParamHolder` class.

Once all your parameters are created, create the GUI for them using the :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>` function::

    panel = NatronGui.PyPanel("fr.inria.mypanel","My Panel",True,app)
    myInteger = panel.createIntParam("myInt","This is an integer very important")
    myInteger.setAnimationEnabled(False)
    myInteger.setAddNewLine(False)

    #Create a boolean on the same line
    myBoolean = panel.createBooleanParam("myBool","Yet another important boolean")

    panel.refreshUserParamsGUI()


You can then retrieve the value of a parameter at any time using the :func:`getParam(scriptName)<NatronGui.PyPanel.getParam>` function::

    intValue = panel.getParam("myInt").get()
    boolValue = panel.getParam("myBool").get()

.. warning::

    Unlike the :ref:`Effect<Effect>` class, parameters on panels are not automatically declared by Natron,
    which means you cannot do stuff like *panel.intValue*

You can get notified when a parameter's value changed, by setting a callback using the
:func:`setParamChangedCallback(callback)<NatronGui.PyPanel.setParamChangedCallback>` function that takes
the name of a Python-defined function in parameters.
The variable **thisParam** will be declared prior to calling the callback, referencing the parameter
which just had its value changed.




Managing the panel:
^^^^^^^^^^^^^^^^^^^

Once created, you must add your panel to a :doc:`PyTabWidget` so it can be visible.
Use the :func:`getTabWidget(scriptName)<NatronGui.GuiApp.getTabWidget>` function to get a
particular pane and then use the :func:`appendTab(tab)<NatronGui.PyTabWidget.appendTab>` function
to add this panel to the pane.

.. warning::

    Note that the lifetime of the widget will be by default the same as the project's GUI
    because :doc:`PyPanel` is :ref:`auto-declared<autoVar>` by Natron.

::

    panel = NatronGui.PyPanel("fr.inria.mypanel","My Panel",True,app)
    ...
    ...
    pane = app.getTabWidget("pane1")
    pane.appendTab(panel)
    app.mypanel = panel


If you want the panel to persist in the project so that it gets recreated and placed at its original position
when the user loads the project, you must use the :func:`registerPythonPanel(panel,function)< NatronGui.GuiApp.registerPythonPanel>` function.

Note that the *function* parameter is the **name** of a Python-defined function that takes no parameter used to create the widget, e.g.:

    def createMyPanel():
        panel = NatronGui.PyPanel("MyPanel",True,app)
        ...
        #Make it live after the scope of the function
        app.mypanel = panel

    app.registerPythonPanel(app.mypanel,"createMyPanel")

This function will also add a custom menu entry to the "Manage layout" button (located in the top-left hand
corner of every pane) which the user can trigger to move the custom pane on the selected pane.

.. figure:: ../../customPaneEntry.png
    :width: 600px
    :align: center

.. _panelSerialization:

Saving and restoring state:
^^^^^^^^^^^^^^^^^^^^^^^^^^^

When the panel is registered in the project using the  :func:`registerPythonPanel(panel,function)<NatronGui.GuiApp.registerPythonPanel>` function,
you may want to also save the state of your widgets and/or special values.

To do so, you must sub-class :class:`PyPanel` and implement the :func:`save()<NatronGui.PyPanel.save>` and
:func:`restore(data)<NatronGui.PyPanel.restore>` functions.

.. note::

    User parameters, if used, will be automatically saved and restored, you don't have to save it yourself.
    Hence if the panel is only composed of user parameters that you want to save, you do not need to sub-class
    PyPanel as it will be done automatically for you.

The function :func:`save()<NatronGui.PyPanel.save>` should return a :class:`string` containing the serialization of your
custom data.

The function :func:`restore(data)<NatronGui.PyPanel.restore>` will be called upon loading of a project containing
an instance of your panel. You should then restore the state of the panel from your custom serialized data.

Note that the auto-save of Natron occurs in a separate thread and for this reason it cannot call directly
your :func:`save()<NatronGui.PyPanel.save>` function because it might create a race condition if the user is
actively modifying the user interface using the main-thread.

To overcome this, Natron has an hidden thread-safe way to recover the data you have serialized using the :func:`save()<NatronGui.PyPanel.save>` function.
The downside is that you have to call the :func:`onUserDataChanged()<NatronGui.PyPanel.onUserDataChanged>` function whenever
a value that you want to be persistent has changed (unless this is a user parameter in which case you do not need to call it).

.. warning ::

    If you do not call   :func:`onUserDataChanged()<NatronGui.PyPanel.onUserDataChanged>`, the :func:`save()<NatronGui.PyPanel.save>` function
    will never be called, and the data never serialized.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronGui.PyPanel.PyPanel(label,useUserParameters,app)

    :param label: :class:`str`
    :param useUserParameters: :class:`bool`
    :param app: :class:`GuiApp<NatronGui.GuiApp>`

Make a new PyPanel with the given *label* that will be used to display in the tab header.
If *useUserParameters* is True then user parameters support will be activated,
attempting to modify the underlying layout in these circumstances will result in undefined behaviour.

.. method:: NatronGui.PyPanel.addWidget(widget)

    :param widget: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`

Append a `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>` inherited *widget* at the bottom of the dialog. This allows to add custom GUI created directly using PySide
that will be inserted **after** any custom parameter.

.. warning::

    This function should be used exclusively when the widget was created using *useUserParameters = True*




.. method:: NatronGui.PyPanel.getParam(scriptName)

    :param scriptName: :class:`str`
    :rtype: :class:`Param<NatronEngine.Param>`

Returns the user parameter with the given *scriptName* if it exists or *None* otherwise.

.. warning::

    This function should be used exclusively when the widget was created using *useUserParameters = True*


.. method:: NatronGui.PyPanel.getParams()

    :rtype: :class:`sequence`

Returns all the user parameters used by the panel.

.. warning::

    This function should be used exclusively when the widget was created using *useUserParameters = True*




.. method:: NatronGui.PyPanel.insertWidget(index,widget)

    :param index: :class:`int`
    :param widget: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`

Inserts a `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>` inherited *widget* at the given *index* of the layout in the dialog. This allows to add custom GUI created directly using PySide.
The widget will always be inserted **after** any user parameter.

.. warning::

    This function should be used exclusively when the widget was created using *useUserParameters = True*




.. method:: NatronGui.PyPanel.setParamChangedCallback(callback)

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

    panel.setParamChangedCallback("myParamChangedCallback")

.. warning::

    This function should be used exclusively when the widget was created using *useUserParameters = True*


.. method:: NatronGui.PyPanel.setPanelLabel(label)

    :param callback: :class:`str`

Set the label of the panel as it will be displayed on the tab header of the :doc:`PyTabWidget`.
This name should be unique.

.. method:: NatronGui.PyPanel.getPanelLabel()

    :rtype: :class:`str`

Get the label of the panel as displayed on the tab header of the :doc:`PyTabWidget`.


.. method:: NatronGui.PyPanel.getPanelScriptName()

    :rtype: :class:`str`

Get the script-name of the panel as used internally. This is a unique string identifying the tab in Natron.



.. method:: NatronGui.PyPanel.onUserDataChanged()

Callback to be called whenever a parameter/value (that is not a user parameter) that you want to be
saved has changed.

.. warning ::

    If you do not call   :func:`onUserDataChanged()<NatronGui.PyPanel.onUserDataChanged>`, the :func:`save()NatronGui.PyPanel.save` function
    will never be called, and the data never serialized.


.. warning::

    This function should be used exclusively when the widget was created using *useUserParameters = True*



.. method:: NatronGui.PyPanel.save()

    :rtype: :class:`str`

.. warning::

    You should overload this function in a derived class. The base version does nothing.

.. note::

    User parameters, if used, will be automatically saved and restored, you don't have to save it yourself.
    Hence if the panel is only composed of user parameters that you want to save, you do not need to sub-class
    PyPanel as it will be done automatically for you.

Returns a string with the serialization of your custom data you need to be persistent.

.. method:: NatronGui.PyPanel.restore(data)

    :param data: :class:`str`

.. warning::

    You should overload this function in a derived class. The base version does nothing.

This function should restore the state of your custom :doc:`PyPanel` using the custom *data*
that you serialized.
The *data* are exactly the return value that was returned from the :func:`save()<NatronGui.PyPanel.save>`  function.
