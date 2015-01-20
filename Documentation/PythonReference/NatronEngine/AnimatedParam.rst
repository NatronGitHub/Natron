.. module:: NatronEngine
.. _AnimatedParam:

AnimatedParam
*************

.. inheritance-diagram:: AnimatedParam
    :parts: 2

**Inherited by:** :ref:`StringParamBase`, :ref:`PathParam`, :ref:`OutputFileParam`, :ref:`FileParam`, :ref:`StringParam`, :ref:`BooleanParam`, :ref:`ChoiceParam`, :ref:`ColorParam`, :ref:`DoubleParam`, :ref:`Double2DParam`, :ref:`Double3DParam`, :ref:`IntParam`, :ref:`Int2DParam`, :ref:`Int3DParam`

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`deleteValueAtTime<NatronEngine.AnimatedParam.deleteValueAtTime>` (time[, dimension=0])
*    def :meth:`getCurrentTime<NatronEngine.AnimatedParam.getCurrentTime>` ()
*    def :meth:`getDerivativeAtTime<NatronEngine.AnimatedParam.getDerivativeAtTime>` (time[, dimension=0])
*    def :meth:`getExpression<NatronEngine.AnimatedParam.getExpression>` (dimension)
*    def :meth:`getIntegrateFromTimeToTime<NatronEngine.AnimatedParam.getIntegrateFromTimeToTime>` (time1, time2[, dimension=0])
*    def :meth:`getIsAnimated<NatronEngine.AnimatedParam.getIsAnimated>` ([dimension=0])
*    def :meth:`getKeyIndex<NatronEngine.AnimatedParam.getKeyIndex>` (time[, dimension=0])
*    def :meth:`getKeyTime<NatronEngine.AnimatedParam.getKeyTime>` (index, dimension)
*    def :meth:`getNumKeys<NatronEngine.AnimatedParam.getNumKeys>` ([dimension=0])
*    def :meth:`removeAnimation<NatronEngine.AnimatedParam.removeAnimation>` ([dimension=0])
*    def :meth:`setExpression<NatronEngine.AnimatedParam.setExpression>` (expr, hasRetVariable[, dimension=0])


Detailed Description
--------------------






.. method:: NatronEngine.AnimatedParam.deleteValueAtTime(time[, dimension=0])


    :param time: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.AnimatedParam.getCurrentTime()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.AnimatedParam.getDerivativeAtTime(time[, dimension=0])


    :param time: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.AnimatedParam.getExpression(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: PyObject






.. method:: NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(time1, time2[, dimension=0])


    :param time1: :class:`PySide.QtCore.double`
    :param time2: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.AnimatedParam.getIsAnimated([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.AnimatedParam.getKeyIndex(time[, dimension=0])


    :param time: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.AnimatedParam.getKeyTime(index, dimension)


    :param index: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: PyObject






.. method:: NatronEngine.AnimatedParam.getNumKeys([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.AnimatedParam.removeAnimation([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.AnimatedParam.setExpression(expr, hasRetVariable[, dimension=0])


    :param expr: :class:`NatronEngine.std::string`
    :param hasRetVariable: :class:`PySide.QtCore.bool`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.bool`







