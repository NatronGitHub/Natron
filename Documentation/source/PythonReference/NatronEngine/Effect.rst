.. module:: NatronEngine
.. _Effect:

Effect
******

**Inherits**: :doc:`Group`

Synopsis
--------

This object represents a single node in Natron, that is: an instance of a plug-in.
See :ref:`details`

Functions
^^^^^^^^^

*    def :meth:`allowEvaluation<NatronEngine.Effect.allowEvaluation>` ()
*    def :meth:`blockEvaluation<NatronEngine.Effect.blockEvaluation>` ()
*    def :meth:`canSetInput<NatronEngine.Effect.canSetInput>` (inputNumber, node)
*    def :meth:`connectInput<NatronEngine.Effect.connectInput>` (inputNumber, input)
*    def :meth:`createBooleanParam<NatronEngine.Effect.createBooleanParam>` (name, label)
*    def :meth:`createButtonParam<NatronEngine.Effect.createButtonParam>` (name, label)
*    def :meth:`createChild<NatronEngine.Effect.createChild>` ()
*    def :meth:`createChoiceParam<NatronEngine.Effect.createChoiceParam>` (name, label)
*    def :meth:`createColorParam<NatronEngine.Effect.createColorParam>` (name, label, useAlpha)
*    def :meth:`createDouble2DParam<NatronEngine.Effect.createDouble2DParam>` (name, label)
*    def :meth:`createDouble3DParam<NatronEngine.Effect.createDouble3DParam>` (name, label)
*    def :meth:`createDoubleParam<NatronEngine.Effect.createDoubleParam>` (name, label)
*    def :meth:`createFileParam<NatronEngine.Effect.createFileParam>` (name, label)
*    def :meth:`createGroupParam<NatronEngine.Effect.createGroupParam>` (name, label)
*    def :meth:`createInt2DParam<NatronEngine.Effect.createInt2DParam>` (name, label)
*    def :meth:`createInt3DParam<NatronEngine.Effect.createInt3DParam>` (name, label)
*    def :meth:`createIntParam<NatronEngine.Effect.createIntParam>` (name, label)
*    def :meth:`createOutputFileParam<NatronEngine.Effect.createOutputFileParam>` (name, label)
*    def :meth:`createPageParam<NatronEngine.Effect.createPageParam>` (name, label)
*    def :meth:`createParametricParam<NatronEngine.Effect.createParametricParam>` (name, label, nbCurves)
*    def :meth:`createPathParam<NatronEngine.Effect.createPathParam>` (name, label)
*    def :meth:`createStringParam<NatronEngine.Effect.createStringParam>` (name, label)
*    def :meth:`removeParam<NatronEngine.Effect.removeParam>` (param)
*    def :meth:`destroy<NatronEngine.Effect.destroy>` ([autoReconnect=true])
*    def :meth:`disconnectInput<NatronEngine.Effect.disconnectInput>` (inputNumber)
*    def :meth:`getColor<NatronEngine.Effect.getColor>` ()
*    def :meth:`getCurrentTime<NatronEngine.Effect.getCurrentTime>` ()
*    def :meth:`getInput<NatronEngine.Effect.getInput>` (inputNumber)
*    def :meth:`getLabel<NatronEngine.Effect.getLabel>` ()
*    def :meth:`getInputLabel<NatronEngine.Effect.getInputLabel>` (inputNumber)
*    def :meth:`getMaxInputCount<NatronEngine.Effect.getMaxInputCount>` ()
*    def :meth:`getParam<NatronEngine.Effect.getParam>` (name)
*    def :meth:`getParams<NatronEngine.Effect.getParams>` ()
*    def :meth:`getPluginID<NatronEngine.Effect.getPluginID>` ()
*    def :meth:`getPosition<NatronEngine.Effect.getPosition>` ()
*    def :meth:`getRotoContext<NatronEngine.Effect.getRotoContext>` ()
*    def :meth:`getScriptName<NatronEngine.Effect.getScriptName>` ()
*    def :meth:`getSize<NatronEngine.Effect.getSize>` ()
*    def :meth:`getUserPageParam<NatronEngine.Effect.getUserPageParam>` ()
*    def :meth:`isNull<NatronEngine.Effect.isNull>` ()
*    def :meth:`refreshUserParamsGUI<NatronEngine.Effect.refreshUserParamsGUI>` ()
*    def :meth:`setColor<NatronEngine.Effect.setColor>` (r, g, b)
*    def :meth:`setLabel<NatronEngine.Effect.setLabel>` (name)
*    def :meth:`setPosition<NatronEngine.Effect.setPosition>` (x, y)
*    def :meth:`setScriptName<NatronEngine.Effect.setScriptName>` (scriptName)
*    def :meth:`setSize<NatronEngine.Effect.setSize>` (w, h)


.. _details:

Detailed Description
--------------------


The Effect object can be used to operate with a single node in Natron. 
To create a new Effect, use the :func:`app.createNode(pluginID)<NatronEngine.App.createNode>` function.
    
Natron automatically declares a variable to Python when a new Effect is created. 
This variable will have a script-name determined by Natron as explained in the 
:ref:`autovar` section.

Once an Effect is instantiated, it declares all its :doc:`Param` and inputs. 
See how to :ref:`manage <userParams>` user parameters below 

To get a specific :doc:`Param` by script-name, call the 
:func:`getParam(name) <NatronEngine.Effect.getParam>` function

Input effects are mapped against a zero-based index. To retrieve an input Effect
given an index, you can use the :func:`getInput(inputNumber) <NatronEngine.Effect.getInput>`
function. 
	
To manage inputs, you can connect them and disconnect them with respect to their input
index with the :func:`connectInput(inputNumber,input)<NatronEngine.Effect.connectInput>` and
then :func:`disconnectInput(inputNumber)<NatronEngine.Effect.disconnectInput>` functions.

If you need to destroy permanently the Effect, just call :func:`destroy() <NatronEngine.Effect.destroy()>`.

For convenience some GUI functionalities have been made accessible via the Effect class
to control the GUI of the node (on the node graph):
	
	* Get/Set the node position with the :func:`setPosition(x,y)<NatronEngine.Effect.setPosition>` and :func:`getPosition()<NatronEngine.Effect.getPosition>` functions
	* Get/Set the node size with the :func:`setSize(width,height)<NatronEngine.Effect.setSize>` and :func:`getSize()<NatronEngine.Effect.getSize`> functions
	* Get/Set the node color with the :func:`setColor(r,g,b)<NatronEngine.Effect.setColor>` and :func:`getColor()<NatronEngine.Effect.getColor>` functions
	
.. _userParams:

Creating user parameters
^^^^^^^^^^^^^^^^^^^^^^^^

To create a new user :doc:`parameter<Param>` on the Effect, use one of the **createXParam**
function. To remove a user parameter created, use the :func:`removeParam(param)<NatronEngine.Effect.removeParam>`
function. Note that this function can only be used to remove **user parameters** and cannot
be used to remove parameters that were defined by the OpenFX plug-in.

Once you have made modifications to the user parameters, you must call the 
:func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>` function to notify
the GUI, otherwise no change will appear on the GUI. 


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Effect.allowEvaluation()

	Allows all evaluation (=renders and callback onParamChanged) that would be issued due to
	a call to :func:`setValue<NatronEngine.IntParam.setValue>` on any parameter of the Effect.
	
	Typically to change several values at once we bracket the changes like this::
	
		node.blockEvaluation()	
		param1.setValue(...)
		param2.setValue(...)
		param3.setValue(...)
		node.allowEvaluation()
		param4.setValue(...) # This triggers a new render and a call to the onParamChanged callback




.. method:: NatronEngine.Effect.blockEvaluation()

	Blocks all evaluation (=renders and callback onParamChanged) that would be issued due to
	a call to :func:`setValue<NatronEngine.IntParam.setValue>` on any parameter of the Effect.
	See :func:`allowEvaluation()<NatronEngine.Effect.allowEvaluation>`





.. method:: NatronEngine.Effect.canSetInput(inputNumber, node)


    :param inputNumber: :class:`PySide.QtCore.int`
    :param node: :class:`NatronEngine.Effect`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Effect.connectInput(inputNumber, input)


    :param inputNumber: :class:`PySide.QtCore.int`
    :param input: :class:`NatronEngine.Effect`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Effect.createBooleanParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.BooleanParam`






.. method:: NatronEngine.Effect.createButtonParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.ButtonParam`






.. method:: NatronEngine.Effect.createChild()


    :rtype: :class:`NatronEngine.Effect`






.. method:: NatronEngine.Effect.createChoiceParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.ChoiceParam`






.. method:: NatronEngine.Effect.createColorParam(name, label, useAlpha)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :param useAlpha: :class:`PySide.QtCore.bool`
    :rtype: :class:`NatronEngine.ColorParam`






.. method:: NatronEngine.Effect.createDouble2DParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Double2DParam`






.. method:: NatronEngine.Effect.createDouble3DParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Double3DParam`






.. method:: NatronEngine.Effect.createDoubleParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.DoubleParam`






.. method:: NatronEngine.Effect.createFileParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.FileParam`






.. method:: NatronEngine.Effect.createGroupParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.GroupParam`






.. method:: NatronEngine.Effect.createInt2DParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Int2DParam`






.. method:: NatronEngine.Effect.createInt3DParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Int3DParam`






.. method:: NatronEngine.Effect.createIntParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.IntParam`






.. method:: NatronEngine.Effect.createOutputFileParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.OutputFileParam`






.. method:: NatronEngine.Effect.createPageParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.PageParam`






.. method:: NatronEngine.Effect.createParametricParam(name, label, nbCurves)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :param nbCurves: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.ParametricParam`






.. method:: NatronEngine.Effect.createPathParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.PathParam`






.. method:: NatronEngine.Effect.createStringParam(name, label)


    :param name: :class:`NatronEngine.std::string`
    :param label: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.StringParam`


.. method:: NatronEngine.Effect.removeParam(param)


    :param param: :class:`NatronEngine.Param`
    :rtype: :class:`PySide.QtCore.bool`



.. method:: NatronEngine.Effect.destroy([autoReconnect=true])


    :param autoReconnect: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Effect.disconnectInput(inputNumber)


    :param inputNumber: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Effect.getColor()








.. method:: NatronEngine.Effect.getCurrentTime()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Effect.getInput(inputNumber)


    :param inputNumber: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.Effect`



    
    Returns the node at the given input.
    



.. method:: NatronEngine.Effect.getLabel()


    :rtype: :class:`NatronEngine.std::string`



.. method:: NatronEngine.Effect.getInputLabel(inputNumber)


	:param inputNumber: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.std::string`

	Returns the label of the input at the given *inputNumber*. 
	It corresponds to the label displayed on the arrow of the input in the node graph.

.. method:: NatronEngine.Effect.getMaxInputCount()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Effect.getParam(name)


    :param name: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Param`






.. method:: NatronEngine.Effect.getParams()


    :rtype: 






.. method:: NatronEngine.Effect.getPluginID()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Effect.getPosition()








.. method:: NatronEngine.Effect.getRotoContext()


    :rtype: :class:`NatronEngine.Roto`






.. method:: NatronEngine.Effect.getScriptName()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Effect.getSize()








.. method:: NatronEngine.Effect.getUserPageParam()


    :rtype: :class:`NatronEngine.PageParam`






.. method:: NatronEngine.Effect.isNull()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Effect.refreshUserParamsGUI()








.. method:: NatronEngine.Effect.setColor(r, g, b)


    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`






.. method:: NatronEngine.Effect.setLabel(name)


    :param name: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Effect.setPosition(x, y)


    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`






.. method:: NatronEngine.Effect.setScriptName(scriptName)


    :param scriptName: :class:`NatronEngine.std::string`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Effect.setSize(w, h)


    :param w: :class:`PySide.QtCore.double`
    :param h: :class:`PySide.QtCore.double`







