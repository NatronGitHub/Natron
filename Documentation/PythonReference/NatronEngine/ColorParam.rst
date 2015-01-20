.. module:: NatronEngine
.. _ColorParam:

ColorParam
**********

.. inheritance-diagram:: ColorParam
    :parts: 2

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`addAsDependencyOf<NatronEngine.ColorParam.addAsDependencyOf>` (fromExprDimension, param)
*    def :meth:`get<NatronEngine.ColorParam.get>` ()
*    def :meth:`get<NatronEngine.ColorParam.get>` (frame)
*    def :meth:`getDefaultValue<NatronEngine.ColorParam.getDefaultValue>` ([dimension=0])
*    def :meth:`getDisplayMaximum<NatronEngine.ColorParam.getDisplayMaximum>` (dimension)
*    def :meth:`getDisplayMinimum<NatronEngine.ColorParam.getDisplayMinimum>` (dimension)
*    def :meth:`getMaximum<NatronEngine.ColorParam.getMaximum>` ([dimension=0])
*    def :meth:`getMinimum<NatronEngine.ColorParam.getMinimum>` ([dimension=0])
*    def :meth:`getValue<NatronEngine.ColorParam.getValue>` ([dimension=0])
*    def :meth:`getValueAtTime<NatronEngine.ColorParam.getValueAtTime>` (time[, dimension=0])
*    def :meth:`restoreDefaultValue<NatronEngine.ColorParam.restoreDefaultValue>` ([dimension=0])
*    def :meth:`set<NatronEngine.ColorParam.set>` (r, g, b)
*    def :meth:`set<NatronEngine.ColorParam.set>` (r, g, b, a)
*    def :meth:`set<NatronEngine.ColorParam.set>` (r, g, b, a, frame)
*    def :meth:`set<NatronEngine.ColorParam.set>` (r, g, b, frame)
*    def :meth:`setDefaultValue<NatronEngine.ColorParam.setDefaultValue>` (value[, dimension=0])
*    def :meth:`setDisplayMaximum<NatronEngine.ColorParam.setDisplayMaximum>` (maximum[, dimension=0])
*    def :meth:`setDisplayMinimum<NatronEngine.ColorParam.setDisplayMinimum>` (minimum[, dimension=0])
*    def :meth:`setMaximum<NatronEngine.ColorParam.setMaximum>` (maximum[, dimension=0])
*    def :meth:`setMinimum<NatronEngine.ColorParam.setMinimum>` (minimum[, dimension=0])
*    def :meth:`setValue<NatronEngine.ColorParam.setValue>` (value[, dimension=0])
*    def :meth:`setValueAtTime<NatronEngine.ColorParam.setValueAtTime>` (value, time[, dimension=0])


Detailed Description
--------------------






.. method:: NatronEngine.ColorParam.addAsDependencyOf(fromExprDimension, param)


    :param fromExprDimension: :class:`PySide.QtCore.int`
    :param param: :class:`NatronEngine.Param`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.get(frame)


    :param frame: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.ColorTuple`






.. method:: NatronEngine.ColorParam.get()


    :rtype: :class:`NatronEngine.ColorTuple`






.. method:: NatronEngine.ColorParam.getDefaultValue([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.getDisplayMaximum(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.getDisplayMinimum(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.getMaximum([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.getMinimum([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.getValue([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.getValueAtTime(time[, dimension=0])


    :param time: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.restoreDefaultValue([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.set(r, g, b, a, frame)


    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`
    :param a: :class:`PySide.QtCore.double`
    :param frame: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.set(r, g, b, frame)


    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`
    :param frame: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.set(r, g, b, a)


    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`
    :param a: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.set(r, g, b)


    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ColorParam.setDefaultValue(value[, dimension=0])


    :param value: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.setDisplayMaximum(maximum[, dimension=0])


    :param maximum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.setDisplayMinimum(minimum[, dimension=0])


    :param minimum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.setMaximum(maximum[, dimension=0])


    :param maximum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.setMinimum(minimum[, dimension=0])


    :param minimum: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.setValue(value[, dimension=0])


    :param value: :class:`PySide.QtCore.double`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ColorParam.setValueAtTime(value, time[, dimension=0])


    :param value: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`
    :param dimension: :class:`PySide.QtCore.int`







