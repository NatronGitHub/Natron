.. module:: NatronEngine
.. _BezierCurve:

BezierCurve
***********

.. inheritance-diagram:: BezierCurve
    :parts: 2

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`addControlPoint<NatronEngine.BezierCurve.addControlPoint>` (x, y)
*    def :meth:`addControlPointOnSegment<NatronEngine.BezierCurve.addControlPointOnSegment>` (index, t)
*    def :meth:`getActivatedParam<NatronEngine.BezierCurve.getActivatedParam>` ()
*    def :meth:`getColor<NatronEngine.BezierCurve.getColor>` (time)
*    def :meth:`getColorParam<NatronEngine.BezierCurve.getColorParam>` ()
*    def :meth:`getCompositingOperator<NatronEngine.BezierCurve.getCompositingOperator>` ()
*    def :meth:`getCompositingOperatorParam<NatronEngine.BezierCurve.getCompositingOperatorParam>` ()
*    def :meth:`getFeatherDistance<NatronEngine.BezierCurve.getFeatherDistance>` (time)
*    def :meth:`getFeatherDistanceParam<NatronEngine.BezierCurve.getFeatherDistanceParam>` ()
*    def :meth:`getFeatherFallOff<NatronEngine.BezierCurve.getFeatherFallOff>` (time)
*    def :meth:`getFeatherFallOffParam<NatronEngine.BezierCurve.getFeatherFallOffParam>` ()
*    def :meth:`getIsActivated<NatronEngine.BezierCurve.getIsActivated>` (time)
*    def :meth:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` ()
*    def :meth:`getOpacity<NatronEngine.BezierCurve.getOpacity>` (time)
*    def :meth:`getOpacityParam<NatronEngine.BezierCurve.getOpacityParam>` ()
*    def :meth:`getOverlayColor<NatronEngine.BezierCurve.getOverlayColor>` ()
*    def :meth:`getPointMasterTrack<NatronEngine.BezierCurve.getPointMasterTrack>` (index)
*    def :meth:`isCurveFinished<NatronEngine.BezierCurve.isCurveFinished>` ()
*    def :meth:`moveFeatherByIndex<NatronEngine.BezierCurve.moveFeatherByIndex>` (index, time, dx, dy)
*    def :meth:`moveLeftBezierPoint<NatronEngine.BezierCurve.moveLeftBezierPoint>` (index, time, dx, dy)
*    def :meth:`movePointByIndex<NatronEngine.BezierCurve.movePointByIndex>` (index, time, dx, dy)
*    def :meth:`moveRightBezierPoint<NatronEngine.BezierCurve.moveRightBezierPoint>` (index, time, dx, dy)
*    def :meth:`removeControlPointByIndex<NatronEngine.BezierCurve.removeControlPointByIndex>` (index)
*    def :meth:`setActivated<NatronEngine.BezierCurve.setActivated>` (time, activated)
*    def :meth:`setColor<NatronEngine.BezierCurve.setColor>` (time, r, g, b)
*    def :meth:`setCompositingOperator<NatronEngine.BezierCurve.setCompositingOperator>` (op)
*    def :meth:`setCurveFinished<NatronEngine.BezierCurve.setCurveFinished>` (finished)
*    def :meth:`setFeatherDistance<NatronEngine.BezierCurve.setFeatherDistance>` (dist, time)
*    def :meth:`setFeatherFallOff<NatronEngine.BezierCurve.setFeatherFallOff>` (falloff, time)
*    def :meth:`setFeatherPointAtIndex<NatronEngine.BezierCurve.setFeatherPointAtIndex>` (index, time, x, y, lx, ly, rx, ry)
*    def :meth:`setOpacity<NatronEngine.BezierCurve.setOpacity>` (opacity, time)
*    def :meth:`setOverlayColor<NatronEngine.BezierCurve.setOverlayColor>` (r, g, b)
*    def :meth:`setPointAtIndex<NatronEngine.BezierCurve.setPointAtIndex>` (index, time, x, y, lx, ly, rx, ry)
*    def :meth:`slavePointToTrack<NatronEngine.BezierCurve.slavePointToTrack>` (index, trackTime, trackCenter)


Detailed Description
--------------------






.. attribute:: NatronEngine.BezierCurve.CairoOperatorEnum


.. method:: NatronEngine.BezierCurve.addControlPoint(x, y)


    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.addControlPointOnSegment(index, t)


    :param index: :class:`PySide.QtCore.int`
    :param t: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.getActivatedParam()


    :rtype: :class:`NatronEngine.BooleanParam`






.. method:: NatronEngine.BezierCurve.getColor(time)


    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.ColorTuple`






.. method:: NatronEngine.BezierCurve.getColorParam()


    :rtype: :class:`NatronEngine.ColorParam`






.. method:: NatronEngine.BezierCurve.getCompositingOperator()


    :rtype: :attr:`NatronEngine.BezierCurve.CairoOperatorEnum`






.. method:: NatronEngine.BezierCurve.getCompositingOperatorParam()


    :rtype: :class:`NatronEngine.ChoiceParam`






.. method:: NatronEngine.BezierCurve.getFeatherDistance(time)


    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.getFeatherDistanceParam()


    :rtype: :class:`NatronEngine.DoubleParam`






.. method:: NatronEngine.BezierCurve.getFeatherFallOff(time)


    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.getFeatherFallOffParam()


    :rtype: :class:`NatronEngine.DoubleParam`






.. method:: NatronEngine.BezierCurve.getIsActivated(time)


    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.BezierCurve.getNumControlPoints()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.BezierCurve.getOpacity(time)


    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.getOpacityParam()


    :rtype: :class:`NatronEngine.DoubleParam`






.. method:: NatronEngine.BezierCurve.getOverlayColor()


    :rtype: :class:`NatronEngine.ColorTuple`






.. method:: NatronEngine.BezierCurve.getPointMasterTrack(index)


    :param index: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.DoubleParam`






.. method:: NatronEngine.BezierCurve.isCurveFinished()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.BezierCurve.moveFeatherByIndex(index, time, dx, dy)


    :param index: :class:`PySide.QtCore.int`
    :param time: :class:`PySide.QtCore.int`
    :param dx: :class:`PySide.QtCore.double`
    :param dy: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.moveLeftBezierPoint(index, time, dx, dy)


    :param index: :class:`PySide.QtCore.int`
    :param time: :class:`PySide.QtCore.int`
    :param dx: :class:`PySide.QtCore.double`
    :param dy: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.movePointByIndex(index, time, dx, dy)


    :param index: :class:`PySide.QtCore.int`
    :param time: :class:`PySide.QtCore.int`
    :param dx: :class:`PySide.QtCore.double`
    :param dy: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.moveRightBezierPoint(index, time, dx, dy)


    :param index: :class:`PySide.QtCore.int`
    :param time: :class:`PySide.QtCore.int`
    :param dx: :class:`PySide.QtCore.double`
    :param dy: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.removeControlPointByIndex(index)


    :param index: :class:`PySide.QtCore.int`






.. method:: NatronEngine.BezierCurve.setActivated(time, activated)


    :param time: :class:`PySide.QtCore.int`
    :param activated: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.BezierCurve.setColor(time, r, g, b)


    :param time: :class:`PySide.QtCore.int`
    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.setCompositingOperator(op)


    :param op: :attr:`NatronEngine.BezierCurve.CairoOperatorEnum`






.. method:: NatronEngine.BezierCurve.setCurveFinished(finished)


    :param finished: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.BezierCurve.setFeatherDistance(dist, time)


    :param dist: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`






.. method:: NatronEngine.BezierCurve.setFeatherFallOff(falloff, time)


    :param falloff: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`






.. method:: NatronEngine.BezierCurve.setFeatherPointAtIndex(index, time, x, y, lx, ly, rx, ry)


    :param index: :class:`PySide.QtCore.int`
    :param time: :class:`PySide.QtCore.int`
    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`
    :param lx: :class:`PySide.QtCore.double`
    :param ly: :class:`PySide.QtCore.double`
    :param rx: :class:`PySide.QtCore.double`
    :param ry: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.setOpacity(opacity, time)


    :param opacity: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`






.. method:: NatronEngine.BezierCurve.setOverlayColor(r, g, b)


    :param r: :class:`PySide.QtCore.double`
    :param g: :class:`PySide.QtCore.double`
    :param b: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.setPointAtIndex(index, time, x, y, lx, ly, rx, ry)


    :param index: :class:`PySide.QtCore.int`
    :param time: :class:`PySide.QtCore.int`
    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`
    :param lx: :class:`PySide.QtCore.double`
    :param ly: :class:`PySide.QtCore.double`
    :param rx: :class:`PySide.QtCore.double`
    :param ry: :class:`PySide.QtCore.double`






.. method:: NatronEngine.BezierCurve.slavePointToTrack(index, trackTime, trackCenter)


    :param index: :class:`PySide.QtCore.int`
    :param trackTime: :class:`PySide.QtCore.int`
    :param trackCenter: :class:`NatronEngine.DoubleParam`







