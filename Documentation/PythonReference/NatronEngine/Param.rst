.. module:: NatronEngine
.. _Param:

Param
*****

.. inheritance-diagram:: Param
    :parts: 2

**Inherited by:** :ref:`ParametricParam`, :ref:`PageParam`, :ref:`GroupParam`, :ref:`ButtonParam`, :ref:`AnimatedParam`, :ref:`StringParamBase`, :ref:`PathParam`, :ref:`OutputFileParam`, :ref:`FileParam`, :ref:`StringParam`, :ref:`BooleanParam`, :ref:`ChoiceParam`, :ref:`ColorParam`, :ref:`DoubleParam`, :ref:`Double2DParam`, :ref:`Double3DParam`, :ref:`IntParam`, :ref:`Int2DParam`, :ref:`Int3DParam`

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`_addAsDependencyOf<NatronEngine.Param._addAsDependencyOf>` (fromExprDimension, param)
*    def :meth:`getAddNewLine<NatronEngine.Param.getAddNewLine>` ()
*    def :meth:`getCanAnimate<NatronEngine.Param.getCanAnimate>` ()
*    def :meth:`getEvaluateOnChange<NatronEngine.Param.getEvaluateOnChange>` ()
*    def :meth:`getHelp<NatronEngine.Param.getHelp>` ()
*    def :meth:`getIsAnimationEnabled<NatronEngine.Param.getIsAnimationEnabled>` ()
*    def :meth:`getIsEnabled<NatronEngine.Param.getIsEnabled>` ([dimension=0])
*    def :meth:`getIsPersistant<NatronEngine.Param.getIsPersistant>` ()
*    def :meth:`getIsVisible<NatronEngine.Param.getIsVisible>` ()
*    def :meth:`getLabel<NatronEngine.Param.getLabel>` ()
*    def :meth:`getNumDimensions<NatronEngine.Param.getNumDimensions>` ()
*    def :meth:`getParent<NatronEngine.Param.getParent>` ()
*    def :meth:`getScriptName<NatronEngine.Param.getScriptName>` ()
*    def :meth:`getTypeName<NatronEngine.Param.getTypeName>` ()
*    def :meth:`setAddNewLine<NatronEngine.Param.setAddNewLine>` (a)
*    def :meth:`setAnimationEnabled<NatronEngine.Param.setAnimationEnabled>` (e)
*    def :meth:`setEnabled<NatronEngine.Param.setEnabled>` (enabled[, dimension=0])
*    def :meth:`setEvaluateOnChange<NatronEngine.Param.setEvaluateOnChange>` (eval)
*    def :meth:`setHelp<NatronEngine.Param.setHelp>` (help)
*    def :meth:`setPersistant<NatronEngine.Param.setPersistant>` (persistant)
*    def :meth:`setVisible<NatronEngine.Param.setVisible>` (visible)


Detailed Description
--------------------






.. method:: NatronEngine.Param._addAsDependencyOf(fromExprDimension, param)


    :param fromExprDimension: :class:`PySide.QtCore.int`
    :param param: :class:`NatronEngine.Param`






.. method:: NatronEngine.Param.getAddNewLine()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getCanAnimate()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getEvaluateOnChange()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getHelp()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.getIsAnimationEnabled()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getIsEnabled([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getIsPersistant()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getIsVisible()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getLabel()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.getNumDimensions()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Param.getParent()


    :rtype: :class:`NatronEngine.Param`






.. method:: NatronEngine.Param.getScriptName()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.getTypeName()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.setAddNewLine(a)


    :param a: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setAnimationEnabled(e)


    :param e: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setEnabled(enabled[, dimension=0])


    :param enabled: :class:`PySide.QtCore.bool`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Param.setEvaluateOnChange(eval)


    :param eval: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setHelp(help)


    :param help: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.setPersistant(persistant)


    :param persistant: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setVisible(visible)


    :param visible: :class:`PySide.QtCore.bool`







