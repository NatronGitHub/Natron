.. module:: NatronEngine
.. _BezierCurve:

BezierCurve
***********

**Inherits** :doc:`ItemBase`

Synopsis
--------

A BezierCurve is the class used for beziers, ellipses and rectangles.
See :ref:`detailed<bezier.details>` description....

Functions
^^^^^^^^^

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


.. _bezier.details:

Detailed Description
--------------------

Almost all functionalities available to the user have been made available to the Python API,
although in practise making a shape just by calling functions might be tedious due to the 
potential huge number of control points and keyframes. You can use the Natron Group node's export
functionality to generate automatically a script from a Roto node within that group.

A bezier initially is in an *opened* state, meaning it doesn't produce a shape yet. 
At this stage you can then add control points using the :func`addControlPoint(x,y)<NatronEngine.BezierCurve.addControlPoint>`
function.
Once you're one adding control points, call the function :func:`setCurveFinished(finished)<NatronEngine.BezierCurve.setCurveFinished>`
to close the shape by connecting the last control point with the first.

Once finished, you can refine the bezier curve by adding control points with the :func:`addControlPointOnSegment(index,t)<NatronEngine.BezierCurve.addControlPointOnSegment>` function.
You can then move and remove control points of the bezier.

You can also slave a control point to a track using the :func:`slavePointToTrack(index,trackTime,trackCenter)<NatronEngine.BezierCurve.slavePointToTrac>` function.

A bezier curve has several properties that the API allows you to modify:

	* opacity
	* color
	* feather distance
	* feather fall-off
	* enable state
	* overlay color
	* compositing operator
	



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. attribute:: NatronEngine.BezierCurve.CairoOperatorEnum

	This enumeration represents the different blending modes of a shape. See the user interface
	for the different modes, or type help(NatronEngine.BezierCurve.CairoOperatorEnum) to see
	the different values.


.. method:: NatronEngine.BezierCurve.addControlPoint(x, y)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`


Adds a new control point to an *opened* shape (see :func:`isCurveFinished()<NatronEngine.BezierCurve.isCurveFinished>`) at coordinates (x,y). 
By default the feather point attached to this point will be equivalent to the control point.
If the auto-keying is enabled in the user interface, then this function will set a keyframe at
the timeline's current time for this shape.



.. method:: NatronEngine.BezierCurve.addControlPointOnSegment(index, t)


    :param index: :class:`PySide.QtCore.int`
    :param t: :class:`PySide.QtCore.double`

Adds a new control point to a *closed* shape (see :func:`isCurveFinished()<NatronEngine.BezierCurve.isCurveFinished>`).
The *index* is the index of the bezier segment linking the control points at *index* and *index + 1*.
*t* is a value between [0,1] indicating the distance from the control point *index* the new control point should be.
The closer to 1 *t* is, the closer the new control point will be to the control point at *index +1*.
By default the feather point attached to this point will be equivalent to the control point.

If the auto-keying is enabled in the user interface, then this function will set a keyframe at
the timeline's current time for this shape.


.. method:: NatronEngine.BezierCurve.getActivatedParam()


    :rtype: :class:`BooleanParam<NatronEngine.BooleanParam>`

Returns the :doc:`Param` controlling the enabled state of the bezier.




.. method:: NatronEngine.BezierCurve.getColor(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`ColorTuple<NatronEngine.ColorTuple>`

Returns the value of the color parameter at the given time as an [R,G,B,A] tuple. Note that
alpha will always be 1.



.. method:: NatronEngine.BezierCurve.getColorParam()


    :rtype: :class:`ColorParam<NatronEngine.ColorParam>`

Returns the :doc:`Param` controlling the color of the bezier.




.. method:: NatronEngine.BezierCurve.getCompositingOperator()


    :rtype: :attr:`NatronEngine.BezierCurve.CairoOperatorEnum`


Returns the blending mode for this shape. Type help(NatronEngine.BezierCurve.CairoOperatorEnum)
to see the different values possible.



.. method:: NatronEngine.BezierCurve.getCompositingOperatorParam()


    :rtype: :class:`NatronEngine.ChoiceParam`


Returns the :doc:`Param` controlling the blending mode of the bezier.




.. method:: NatronEngine.BezierCurve.getFeatherDistance(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`


Returns the feather distance of this shape at the given *time*.



.. method:: NatronEngine.BezierCurve.getFeatherDistanceParam()


    :rtype: :class:`NatronEngine.DoubleParam`


Returns the :doc:`Param` controlling the feather distance of the bezier.




.. method:: NatronEngine.BezierCurve.getFeatherFallOff(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`


Returns the feather fall-off of this shape at the given *time*.



.. method:: NatronEngine.BezierCurve.getFeatherFallOffParam()


    :rtype: :class:`DoubleParam<NatronEngine.DoubleParam>`


Returns the :doc:`Param` controlling the color of the bezier.




.. method:: NatronEngine.BezierCurve.getIsActivated(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the curve is enabled or not at the given *time*. When
not activated the curve will not be rendered at all in the image.



.. method:: NatronEngine.BezierCurve.getNumControlPoints()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of control points for this shape.




.. method:: NatronEngine.BezierCurve.getOpacity(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`

Returns the opacity of the curve at the given *time*.



.. method:: NatronEngine.BezierCurve.getOpacityParam()


    :rtype: :class:`DoubleParam<NatronEngine.DoubleParam>`


Returns the :doc:`Param` controlling the opacity of the bezier.



.. method:: NatronEngine.BezierCurve.getOverlayColor()


    :rtype: :class:`ColorTuple<NatronEngine.ColorTuple>`


Returns the overlay color of this shape as a [R,G,B,A] tuple. Alpha will always be 1.



.. method:: NatronEngine.BezierCurve.getPointMasterTrack(index)


    :param index: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`DoubleParam<NatronEngine.DoubleParam>`



Returns the :doc:`Param` of the center of the track controlling the control point at 
the given *index*.



.. method:: NatronEngine.BezierCurve.isCurveFinished()


    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the curve is finished or not. A finished curve will have a bezier segment between
the last control point and the first control point and the bezier will be rendered in the image.



.. method:: NatronEngine.BezierCurve.moveFeatherByIndex(index, time, dx, dy)


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`

Moves the feather point at the given *index* (zero-based) by the given delta (dx,dy). 
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.




.. method:: NatronEngine.BezierCurve.moveLeftBezierPoint(index, time, dx, dy)


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`


Moves the left bezier point of the control point at the given *index* by the given delta.
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.


.. method:: NatronEngine.BezierCurve.movePointByIndex(index, time, dx, dy)


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`

Moves the point at the given *index* (zero-based) by the given delta (dx,dy). 
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.




.. method:: NatronEngine.BezierCurve.moveRightBezierPoint(index, time, dx, dy)


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`

Moves the right bezier point at the given *index* (zero-based) by the given delta (dx,dy). 
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.




.. method:: NatronEngine.BezierCurve.removeControlPointByIndex(index)


    :param index: :class:`int<PySide.QtCore.int>`

Removes the control point at the given *index* (zero-based).




.. method:: NatronEngine.BezierCurve.setActivated(time, activated)


    :param time: :class:`int<PySide.QtCore.int>`
    :param activated: :class:`bool<PySide.QtCore.bool>`


Set a new keyframe for the *activated* parameter at the given *time*


.. method:: NatronEngine.BezierCurve.setColor(time, r, g, b)


    :param time: :class:`int<PySide.QtCore.int>`
    :param r: :class:`float<PySide.QtCore.double>`
    :param g: :class:`float<PySide.QtCore.double>`
    :param b: :class:`float<PySide.QtCore.double>`


Set a new keyframe for the *color* parameter at the given *time*




.. method:: NatronEngine.BezierCurve.setCompositingOperator(op)


    :param op: :attr:`NatronEngine.BezierCurve.CairoOperatorEnum`


Set the compositing operator for this shape.



.. method:: NatronEngine.BezierCurve.setCurveFinished(finished)


    :param finished: :class:`bool<PySide.QtCore.bool>`


Set whether the curve should be finished or not. See :func:`isCurveFinished()<NatronEngine.BezierCurve.isCurveFinished>`



.. method:: NatronEngine.BezierCurve.setFeatherDistance(dist, time)


    :param dist: :class:`float<PySide.QtCore.double>`
    :param time: :class:`int<PySide.QtCore.int>`



Set a new keyframe for the *feather distance* parameter at the given *time*


.. method:: NatronEngine.BezierCurve.setFeatherFallOff(falloff, time)


    :param falloff: :class:`float<PySide.QtCore.double>`
    :param time: :class:`int<PySide.QtCore.int>`


Set a new keyframe for the *feather fall-off* parameter at the given *time*



.. method:: NatronEngine.BezierCurve.setFeatherPointAtIndex(index, time, x, y, lx, ly, rx, ry)


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param lx: :class:`float<PySide.QtCore.double>`
    :param ly: :class:`float<PySide.QtCore.double>`
    :param rx: :class:`float<PySide.QtCore.double>`
    :param ry: :class:`float<PySide.QtCore.double>`

Set the feather point at the given *index* at  the position (x,y) with the left bezier point
at (lx,ly) and right bezier point at (rx,ry).

The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.


.. method:: NatronEngine.BezierCurve.setOpacity(opacity, time)


    :param opacity: :class:`float<PySide.QtCore.double>`
    :param time: :class:`int<PySide.QtCore.int>`


Set a new keyframe for the *opacity* parameter at the given *time*



.. method:: NatronEngine.BezierCurve.setOverlayColor(r, g, b)


    :param r: :class:`float<PySide.QtCore.double>`
    :param g: :class:`float<PySide.QtCore.double>`
    :param b: :class:`float<PySide.QtCore.double>`



Set the overlay color of this shape


.. method:: NatronEngine.BezierCurve.setPointAtIndex(index, time, x, y, lx, ly, rx, ry)


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param lx: :class:`float<PySide.QtCore.double>`
    :param ly: :class:`float<PySide.QtCore.double>`
    :param rx: :class:`float<PySide.QtCore.double>`
    :param ry: :class:`float<PySide.QtCore.double>`


Set the point at the given *index* at  the position (x,y) with the left bezier point
at (lx,ly) and right bezier point at (rx,ry).

The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.

.. method:: NatronEngine.BezierCurve.slavePointToTrack(index, trackTime, trackCenter)


    :param index: :class:`int<PySide.QtCore.int>`
    :param trackTime: :class:`int<PySide.QtCore.int>`
    :param trackCenter: :class:`Double2DParam<NatronEngine.Double2DParam>`

Slave the control point at the given *index* to the *trackCenter* :doc:`parameter<Double2DParam>`.
The *trackCenter* must be a 2-dimensional double parameter and must be the parameter called *center* of a track.

Once slaved the control point will move as a relative offset from the track. The offset
is initially 0 at the given *trackTime*.



