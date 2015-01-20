.. module:: NatronEngine
.. _DoubleParam:

DoubleParam
***********

.. inheritance-diagram:: DoubleParam
    :parts: 2

**Inherited by:** :ref:`Double2DParam`, :ref:`Double3DParam`

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`addAsDependencyOf<NatronEngine.DoubleParam.addAsDependencyOf>` (fromExprDimension, param)
*    def :meth:`get<NatronEngine.DoubleParam.get>` ()
*    def :meth:`get<NatronEngine.DoubleParam.get>` (frame)
*    def :meth:`getDefaultValue<NatronEngine.DoubleParam.getDefaultValue>` ([dimension=0])
*    def :meth:`getDisplayMaximum<NatronEngine.DoubleParam.getDisplayMaximum>` (dimension)
*    def :meth:`getDisplayMinimum<NatronEngine.DoubleParam.getDisplayMinimum>` (dimension)
*    def :meth:`getMaximum<NatronEngine.DoubleParam.getMaximum>` ([dimension=0])
*    def :meth:`getMinimum<NatronEngine.DoubleParam.getMinimum>` ([dimension=0])
*    def :meth:`getValue<NatronEngine.DoubleParam.getValue>` ([dimension=0])
*    def :meth:`getValueAtTime<NatronEngine.DoubleParam.getValueAtTime>` (time[, dimension=0])
*    def :meth:`restoreDefaultValue<NatronEngine.DoubleParam.restoreDefaultValue>` ([dimension=0])
*    def :meth:`set<NatronEngine.DoubleParam.set>` (x)
*    def :meth:`set<NatronEngine.DoubleParam.set>` (x, frame)
*    def :meth:`setDefaultValue<NatronEngine.DoubleParam.setDefaultValue>` (value[, dimension=0])
*    def :meth:`setDisplayMaximum<NatronEngine.DoubleParam.setDisplayMaximum>` (maximum[, dimension=0])
*    def :meth:`setDisplayMinimum<NatronEngine.DoubleParam.setDisplayMinimum>` (minimum[, dimension=0])
*    def :meth:`setMaximum<NatronEngine.DoubleParam.setMaximum>` (maximum[, dimension=0])
*    def :meth:`setMinimum<NatronEngine.DoubleParam.setMinimum>` (minimum[, dimension=0])
*    def :meth:`setValue<NatronEngine.DoubleParam.setValue>` (value[, dimension=0])
*    def :meth:`setValueAtTime<NatronEngine.DoubleParam.setValueAtTime>` (value, time[, dimension=0])


Detailed Description
--------------------






.. method:: NatronEngine.DoubleParam.addAsDependencyOf(fromExprDimension, param)


    :param fromExprDimension: :class:`PySide.QtCore.int`
    :param param: :class:`NatronEngine.Param`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.get(frame)


    :param frame: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.get()


    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getDefaultValue([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getDisplayMaximum(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getDisplayMinimum(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getMaximum([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getMinimum([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getValue([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.getValueAtTime(time[, dimension=0])


    :param time: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.restoreDefaultValue([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.set(x, frame)


    :param x: :class:`PySide.QtCore.double`
    :param frame: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.set(x)


    :param x: :class:`PySide.QtCore.double`






.. method:: NatronEngine.DoubleParam.setDefaultValue(value[, dimension=0])


    :param value: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.setDisplayMaximum(maximum[, dimension=0])


    :param maximum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.setDisplayMinimum(minimum[, dimension=0])


    :param minimum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.setMaximum(maximum[, dimension=0])


    :param maximum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.setMinimum(minimum[, dimension=0])


    :param minimum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.setValue(value[, dimension=0])


    :param value: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.DoubleParam.setValueAtTime(value, time[, dimension=0])


    :param value: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`







