.. module:: NatronEngine
.. _Effect:

Effect
******

.. inheritance-diagram:: Effect
    :parts: 2

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

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
*    def :meth:`destroy<NatronEngine.Effect.destroy>` ([autoReconnect=true])
*    def :meth:`disconnectInput<NatronEngine.Effect.disconnectInput>` (inputNumber)
*    def :meth:`getColor<NatronEngine.Effect.getColor>` ()
*    def :meth:`getCurrentTime<NatronEngine.Effect.getCurrentTime>` ()
*    def :meth:`getInput<NatronEngine.Effect.getInput>` (inputNumber)
*    def :meth:`getLabel<NatronEngine.Effect.getLabel>` ()
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


Detailed Description
--------------------


    
    This object represents a single node in Natron, that is: an instance of a plug-in.
    




.. method:: NatronEngine.Effect.allowEvaluation()








.. method:: NatronEngine.Effect.blockEvaluation()








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







