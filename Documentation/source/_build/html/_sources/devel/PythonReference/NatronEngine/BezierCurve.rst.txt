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

- def :meth:`addControlPoint<NatronEngine.BezierCurve.addControlPoint>` (x, y)
- def :meth:`addControlPointOnSegment<NatronEngine.BezierCurve.addControlPointOnSegment>` (index, t)
- def :meth:`getActivatedParam<NatronEngine.BezierCurve.getActivatedParam>` ()
- def :meth:`getColor<NatronEngine.BezierCurve.getColor>` (time)
- def :meth:`getColorParam<NatronEngine.BezierCurve.getColorParam>` ()
- def :meth:`getCompositingOperator<NatronEngine.BezierCurve.getCompositingOperator>` ()
- def :meth:`getCompositingOperatorParam<NatronEngine.BezierCurve.getCompositingOperatorParam>` ()
- def :meth:`getControlPointPosition<NatronEngine.BezierCurve.getControlPointPosition>` (index,time)
- def :meth:`getFeatherDistance<NatronEngine.BezierCurve.getFeatherDistance>` (time)
- def :meth:`getFeatherDistanceParam<NatronEngine.BezierCurve.getFeatherDistanceParam>` ()
- def :meth:`getFeatherFallOff<NatronEngine.BezierCurve.getFeatherFallOff>` (time)
- def :meth:`getFeatherFallOffParam<NatronEngine.BezierCurve.getFeatherFallOffParam>` ()
- def :meth:`getFeatherPointPosition<NatronEngine.BezierCurve.getControlPointPosition>` (index,time)
- def :meth:`getIsActivated<NatronEngine.BezierCurve.getIsActivated>` (time)
- def :meth:`getKeyframes<NatronEngine.BezierCurve.getKeyframes>` ()
- def :meth:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` ()
- def :meth:`getOpacity<NatronEngine.BezierCurve.getOpacity>` (time)
- def :meth:`getOpacityParam<NatronEngine.BezierCurve.getOpacityParam>` ()
- def :meth:`getOverlayColor<NatronEngine.BezierCurve.getOverlayColor>` ()
- def :meth:`isCurveFinished<NatronEngine.BezierCurve.isCurveFinished>` ()
- def :meth:`moveFeatherByIndex<NatronEngine.BezierCurve.moveFeatherByIndex>` (index, time, dx, dy)
- def :meth:`moveLeftBezierPoint<NatronEngine.BezierCurve.moveLeftBezierPoint>` (index, time, dx, dy)
- def :meth:`movePointByIndex<NatronEngine.BezierCurve.movePointByIndex>` (index, time, dx, dy)
- def :meth:`moveRightBezierPoint<NatronEngine.BezierCurve.moveRightBezierPoint>` (index, time, dx, dy)
- def :meth:`removeControlPointByIndex<NatronEngine.BezierCurve.removeControlPointByIndex>` (index)
- def :meth:`setActivated<NatronEngine.BezierCurve.setActivated>` (time, activated)
- def :meth:`setColor<NatronEngine.BezierCurve.setColor>` (time, r, g, b)
- def :meth:`setCompositingOperator<NatronEngine.BezierCurve.setCompositingOperator>` (op)
- def :meth:`setCurveFinished<NatronEngine.BezierCurve.setCurveFinished>` (finished)
- def :meth:`setFeatherDistance<NatronEngine.BezierCurve.setFeatherDistance>` (dist, time)
- def :meth:`setFeatherFallOff<NatronEngine.BezierCurve.setFeatherFallOff>` (falloff, time)
- def :meth:`setFeatherPointAtIndex<NatronEngine.BezierCurve.setFeatherPointAtIndex>` (index, time, x, y, lx, ly, rx, ry)
- def :meth:`setOpacity<NatronEngine.BezierCurve.setOpacity>` (opacity, time)
- def :meth:`setOverlayColor<NatronEngine.BezierCurve.setOverlayColor>` (r, g, b)
- def :meth:`setPointAtIndex<NatronEngine.BezierCurve.setPointAtIndex>` (index, time, x, y, lx, ly, rx, ry)


.. _bezier.details:

Detailed Description
--------------------

Almost all functionalities available to the user have been made available to the Python API,
although in practise making a shape just by calling functions might be tedious due to the
potential huge number of control points and keyframes. You can use the Natron Group node's export
functionality to generate automatically a script from a Roto node within that group.

A Bezier initially is in an *opened* state, meaning it doesn't produce a shape yet.
At this stage you can then add control points using the :func:`addControlPoint(x,y)<NatronEngine.BezierCurve.addControlPoint>`
function.
Once you're one adding control points, call the function :func:`setCurveFinished(finished)<NatronEngine.BezierCurve.setCurveFinished>`
to close the shape by connecting the last control point with the first.

Once finished, you can refine the Bezier curve by adding control points with the :func:`addControlPointOnSegment(index,t)<NatronEngine.BezierCurve.addControlPointOnSegment>` function.
You can then move and remove control points of the Bezier.

To get the position of the control points of the Bezier as well as the position of the feather
points, use the functions :func:`getControlPointPosition<NatronEngine.BezierCurve.getControlPointPosition>` and
:func:`getFeatherPointPosition<NatronEngine.BezierCurve.getFeatherPointPosition>`.
The *index* passed to the function must be between 0 and :func:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` -1.

The *time* passed to the function corresponds to a time on the timeline's in frames.
If it lands on a keyframe of the Bezier shape, then the position at that keyframe is returned,
otherwise the position is sampled between the surrounding keyframes.

To get a list of all keyframes time for a Bezier call the function :func:`getKeyframes()<NatronEngine.BezierCurve.getKeyframes>`.

A Bezier curve has several parameters that the API allows you to modify:

- opacity
- color
- feather distance
- feather fall-off
- enable state
- overlay color
- compositing operator

Each of them is a regular :class:`Param<NatronEngine.Param>` that you can access to modify
or query its properties.
All parameters can be retrieved with their *script-name* with the function :func:`getParam(scriptName)<NatronEngine.ItemBase.getParam>`.



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
The *index* is the index of the Bezier segment linking the control points at *index* and *index + 1*.
*t* is a value between [0,1] indicating the distance from the control point *index* the new control point should be.
The closer to 1 *t* is, the closer the new control point will be to the control point at *index +1*.
By default the feather point attached to this point will be equivalent to the control point.

If the auto-keying is enabled in the user interface, then this function will set a keyframe at
the timeline's current time for this shape.


.. method:: NatronEngine.BezierCurve.getActivatedParam()


    :rtype: :class:`BooleanParam<NatronEngine.BooleanParam>`

Returns the :doc:`Param` controlling the enabled state of the Bezier.




.. method:: NatronEngine.BezierCurve.getColor(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`ColorTuple<NatronEngine.ColorTuple>`

Returns the value of the color parameter at the given time as an [R,G,B,A] tuple. Note that
alpha will always be 1.



.. method:: NatronEngine.BezierCurve.getColorParam()


    :rtype: :class:`ColorParam<NatronEngine.ColorParam>`

Returns the :doc:`Param` controlling the color of the Bezier.




.. method:: NatronEngine.BezierCurve.getCompositingOperator()


    :rtype: :attr:`NatronEngine.BezierCurve.CairoOperatorEnum`


Returns the blending mode for this shape. Type help(NatronEngine.BezierCurve.CairoOperatorEnum)
to see the different values possible.



.. method:: NatronEngine.BezierCurve.getCompositingOperatorParam()


    :rtype: :class:`NatronEngine.ChoiceParam`


Returns the :doc:`Param` controlling the blending mode of the Bezier.

.. method:: NatronEngine.BezierCurve.getControlPointPosition(index, time)

    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`PyTuple`

Returns a tuple with the position of the control point at the given *index* as well as the
position of its left and right tangents.

The tuple is encoded as such::

    (x,y, leftTangentX, leftTangentY, rightTangentX, rightTangentY)

The position of the left and right tangents is absolute and not relative to (x,y).

The *index* passed to the function must be between 0 and :func:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` -1.
The *time* passed to the function corresponds to a time on the timeline's in frames.
If it lands on a keyframe of the Bezier shape, then the position at that keyframe is returned,
otherwise the position is sampled between the surrounding keyframes.

To get a list of all keyframes time for a Bezier call the function :func:`getKeyframes()<NatronEngine.BezierCurve.getKeyframes>`.


.. method:: NatronEngine.BezierCurve.getFeatherDistance(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`


Returns the feather distance of this shape at the given *time*.



.. method:: NatronEngine.BezierCurve.getFeatherDistanceParam()


    :rtype: :class:`NatronEngine.DoubleParam`


Returns the :doc:`Param` controlling the feather distance of the Bezier.




.. method:: NatronEngine.BezierCurve.getFeatherFallOff(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`


Returns the feather fall-off of this shape at the given *time*.



.. method:: NatronEngine.BezierCurve.getFeatherFallOffParam()


    :rtype: :class:`DoubleParam<NatronEngine.DoubleParam>`


Returns the :doc:`Param` controlling the color of the Bezier.


.. method:: NatronEngine.BezierCurve.getFeatherPointPosition(index, time)

    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`PyTuple`

Returns a tuple with the position of the feather point at the given *index* as well as the
position of its left and right tangents.

The tuple is encoded as such::

    (x,y, leftTangentX, leftTangentY, rightTangentX, rightTangentY)

The position of the left and right tangents is absolute and not relative to (x,y).

The *index* passed to the function must be between 0 and :func:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` -1.
The *time* passed to the function corresponds to a time on the timeline's in frames.
If it lands on a keyframe of the Bezier shape, then the position at that keyframe is returned,
otherwise the position is sampled between the surrounding keyframes.

To get a list of all keyframes time for a Bezier call the function :func:`getKeyframes()<NatronEngine.BezierCurve.getKeyframes>`.



.. method:: NatronEngine.BezierCurve.getIsActivated(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the curve is enabled or not at the given *time*. When
not activated the curve will not be rendered at all in the image.


.. method:: NatronEngine.BezierCurve.getKeyframes()


    :rtype: :class:`PyList`


Returns a list of all keyframes set on the Bezier animation.


.. method:: NatronEngine.BezierCurve.getNumControlPoints()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of control points for this shape.




.. method:: NatronEngine.BezierCurve.getOpacity(time)


    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`

Returns the opacity of the curve at the given *time*.



.. method:: NatronEngine.BezierCurve.getOpacityParam()


    :rtype: :class:`DoubleParam<NatronEngine.DoubleParam>`


Returns the :doc:`Param` controlling the opacity of the Bezier.



.. method:: NatronEngine.BezierCurve.getOverlayColor()


    :rtype: :class:`ColorTuple<NatronEngine.ColorTuple>`


Returns the overlay color of this shape as a [R,G,B,A] tuple. Alpha will always be 1.



.. method:: NatronEngine.BezierCurve.isCurveFinished()


    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the curve is finished or not. A finished curve will have a Bezier segment between
the last control point and the first control point and the Bezier will be rendered in the image.



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


Moves the left Bezier point of the control point at the given *index* by the given delta.
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

Moves the right Bezier point at the given *index* (zero-based) by the given delta (dx,dy).
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

Set the feather point at the given *index* at  the position (x,y) with the left Bezier point
at (lx,ly) and right Bezier point at (rx,ry).

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


Set the point at the given *index* at  the position (x,y) with the left Bezier point
at (lx,ly) and right Bezier point at (rx,ry).

The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.

