.. module:: NatronEngine
.. _ParametricParam:

ParametricParam
***************

.. inheritance-diagram:: ParametricParam
    :parts: 2

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`addControlPoint<NatronEngine.ParametricParam.addControlPoint>` (dimension, key, value)
*    def :meth:`deleteAllControlPoints<NatronEngine.ParametricParam.deleteAllControlPoints>` (dimension)
*    def :meth:`deleteControlPoint<NatronEngine.ParametricParam.deleteControlPoint>` (dimension, nthCtl)
*    def :meth:`getCurveColor<NatronEngine.ParametricParam.getCurveColor>` (dimension)
*    def :meth:`getNControlPoints<NatronEngine.ParametricParam.getNControlPoints>` (dimension)
*    def :meth:`getNthControlPoint<NatronEngine.ParametricParam.getNthControlPoint>` (dimension, nthCtl)
*    def :meth:`getValue<NatronEngine.ParametricParam.getValue>` (dimension, parametricPosition)
*    def :meth:`setCurveColor<NatronEngine.ParametricParam.setCurveColor>` (dimension, r, g, b)
*    def :meth:`setNthControlPoint<NatronEngine.ParametricParam.setNthControlPoint>` (dimension, nthCtl, key, value, leftDerivative, rightDerivative)


Detailed Description
--------------------






.. method:: NatronEngine.ParametricParam.addControlPoint(dimension, key, value)


    :param dimension: :class:`PySide.QtCore.int`
    :param key: :class:`PySide.QtCore.double`
    :param value: :class:`PySide.QtCore.double`
    :rtype: :attr:`NatronEngine.Natron.StatusEnum`






.. method:: NatronEngine.ParametricParam.deleteAllControlPoints(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :attr:`NatronEngine.Natron.StatusEnum`






.. method:: NatronEngine.ParametricParam.deleteControlPoint(dimension, nthCtl)


    :param dimension: :class:`PySide.QtCore.int`
    :param nthCtl: :class:`PySide.QtCore.int`
    :rtype: :attr:`NatronEngine.Natron.StatusEnum`






.. method:: NatronEngine.ParametricParam.getCurveColor(dimension)


    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ParametricParam.getNControlPoints(dimension)


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.ParametricParam.getNthControlPoint(dimension, nthCtl)


    :param dimension: :class:`PySide.QtCore.int`
    :param nthCtl: :class:`PySide.QtCore.int`
    :rtype: PyObject






.. method:: NatronEngine.ParametricParam.getValue(dimension, parametricPosition)


    :param dimension: :class:`PySide.QtCore.int`
    :param parametricPosition: :class:`PySide.QtCore.double`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ParametricParam.setCurveColor(dimension, r, g, b)


    :param dimension: :class:`PySide.QtCore.int`
    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`






.. method:: NatronEngine.ParametricParam.setNthControlPoint(dimension, nthCtl, key, value, leftDerivative, rightDerivative)


    :param dimension: :class:`PySide.QtCore.int`
    :param nthCtl: :class:`PySide.QtCore.int`
    :param key: :class:`PySide.QtCore.double`
    :param value: :class:`PySide.QtCore.double`
    :param leftDerivative: :class:`PySide.QtCore.double`
    :param rightDerivative: :class:`PySide.QtCore.double`
    :rtype: :attr:`NatronEngine.Natron.StatusEnum`







