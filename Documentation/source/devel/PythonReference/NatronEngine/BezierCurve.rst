.. module:: NatronEngine
.. _BezierCurve:

BezierCurve
***********

**Inherits** :doc:`ItemBase`

Synopsis
--------

A BezierCurve is the class used for Beziers, ellipses and rectangles.
See :ref:`detailed<bezier.details>` description....

Functions
^^^^^^^^^

- def :meth:`addControlPoint<NatronEngine.BezierCurve.addControlPoint>` (x, y[, view = "All"])
- def :meth:`addControlPointOnSegment<NatronEngine.BezierCurve.addControlPointOnSegment>` (index, t[, view = "All"])
- def :meth:`getControlPointPosition<NatronEngine.BezierCurve.getControlPointPosition>` (index,time[, view = "Main"])
- def :meth:`getFeatherPointPosition<NatronEngine.BezierCurve.getFeatherPointPosition>` (index,time[, view = "Main"])
- def :meth:`isActivated<NatronEngine.BezierCurve.isActivated>` (time[, view = "Main"])
- def :meth:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` ([view = "Main"])
- def :meth:`isCurveFinished<NatronEngine.BezierCurve.isCurveFinished>` ([view = "Main"])
- def :meth:`moveFeatherByIndex<NatronEngine.BezierCurve.moveFeatherByIndex>` (index, time, dx, dy[, view = "All"])
- def :meth:`moveLeftBezierPoint<NatronEngine.BezierCurve.moveLeftBezierPoint>` (index, time, dx, dy[, view = "All"])
- def :meth:`movePointByIndex<NatronEngine.BezierCurve.movePointByIndex>` (index, time, dx, dy[, view = "All"])
- def :meth:`moveRightBezierPoint<NatronEngine.BezierCurve.moveRightBezierPoint>` (index, time, dx, dy[, view = "All"])
- def :meth:`removeControlPointByIndex<NatronEngine.BezierCurve.removeControlPointByIndex>` (index[, view = "All"])
- def :meth:`setCurveFinished<NatronEngine.BezierCurve.setCurveFinished>` (finished[, view = "All"])
- def :meth:`setFeatherPointAtIndex<NatronEngine.BezierCurve.setFeatherPointAtIndex>` (index, time, x, y, lx, ly, rx, ry[, view = "All"])
- def :meth:`setPointAtIndex<NatronEngine.BezierCurve.setPointAtIndex>` (index, time, x, y, lx, ly, rx, ry[, view = "All"])
- def :meth:`splitView<NatronEngine.BezierCurve.splitView>` (view)
- def :meth:`unSplitView<NatronEngine.BezierCurve.unSplitView>` (view)
- def :meth:`getViewsList<NatronEngine.BezierCurve.getViewsList>` ()

.. _bezier.details:

Detailed Description
--------------------

A Bezier initially is in an *opened* state, meaning it doesn't produce a shape yet.
At this stage you can then add control points using the :func`addControlPoint(x,y)<NatronEngine.BezierCurve.addControlPoint>`
function.
Once you are done adding control points, call the function :func:`setCurveFinished(finished)<NatronEngine.BezierCurve.setCurveFinished>`
to close the shape by connecting the last control point with the first.

Once finished, you can refine the Bezier curve by adding control points with the :func:`addControlPointOnSegment(index,t)<NatronEngine.BezierCurve.addControlPointOnSegment>` function.
You can then move and remove control points of the Bezier.

To get the position of the control points of the Bezier as well as the position of the feather
points, use the functions :func:`getControlPointPosition<NatronEngine.BezierCurve.getControlPointPosition>` and
:func:`getFeatherPointPosition<NatronEngine.BezierCurve.getFeatherPointPosition>`.
The *index* passed to the function must be between 0 and :func:`getNumControlPoints<NatronEngine.BezierCurve.getNumControlPoints>` -1.

If it lands on a keyframe of the Bezier shape, then the position at that keyframe is returned,
otherwise the position is sampled between the surrounding keyframes.

To get a list of all keyframes time for a Bezier call the function :func:`getUserKeyframes()<NatronEngine.ItemBase.getUserKeyframes>`.

Each property of a Bezier is a  regular :ref:`parameter<NatronEngine.Param>`.
All parameters can be retrieved with their *script-name* with the function :func:`getParam(scriptName)<NatronEngine.ItemBase.getParam>`.



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.BezierCurve.addControlPoint(x, y[, view = "All"])


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Adds a new control point to an *opened* shape (see :func:`isCurveFinished()<NatronEngine.BezierCurve.isCurveFinished>`) at coordinates (x,y).
By default the feather point attached to this point will be equivalent to the control point.
If the auto-keying is enabled in the user interface, then this function will set a keyframe at
the timeline's current time for this shape.



.. method:: NatronEngine.BezierCurve.addControlPointOnSegment(index, t[, view = "All"])


    :param index: :class:`PySide.QtCore.int`
    :param t: :class:`PySide.QtCore.double`
    :param view: :class:`view<PySide.QtCore.QString>`

Adds a new control point to a *closed* shape (see :func:`isCurveFinished()<NatronEngine.BezierCurve.isCurveFinished>`).
The *index* is the index of the Bezier segment linking the control points at *index* and *index + 1*.
*t* is a value between [0,1] indicating the distance from the control point *index* the new control point should be.
The closer to 1 *t* is, the closer the new control point will be to the control point at *index +1*.
By default the feather point attached to this point will be equivalent to the control point.

If the auto-keying is enabled in the user interface, then this function will set a keyframe at
the timeline's current time for this shape.


.. method:: NatronEngine.BezierCurve.getControlPointPosition(index, time[, view = "Main"])

    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`view<PySide.QtCore.QString>`
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


.. method:: NatronEngine.BezierCurve.getFeatherPointPosition(index, time[, view = "Main"])

    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`view<PySide.QtCore.QString>`
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



.. method:: NatronEngine.BezierCurve.isActivated(time [, view="Main"])


    :param time: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the curve is enabled or not at the given *time* and *view*. When
not activated the curve will not be rendered at all in the image.

.. method:: NatronEngine.BezierCurve.getNumControlPoints([view = "Main"])

    :param view: :class:`view<PySide.QtCore.QString>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of control points for this shape.



.. method:: NatronEngine.BezierCurve.isCurveFinished([view = "Main"])


    :param view: :class:`view<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the curve is finished or not. A finished curve will have a Bezier segment between
the last control point and the first control point and the Bezier will be rendered in the image.



.. method:: NatronEngine.BezierCurve.moveFeatherByIndex(index, time, dx, dy[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Moves the feather point at the given *index* (zero-based) by the given delta (dx,dy).
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.




.. method:: NatronEngine.BezierCurve.moveLeftBezierPoint(index, time, dx, dy[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Moves the left Bezier point of the control point at the given *index* by the given delta.
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.


.. method:: NatronEngine.BezierCurve.movePointByIndex(index, time, dx, dy[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Moves the point at the given *index* (zero-based) by the given delta (dx,dy).
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.




.. method:: NatronEngine.BezierCurve.moveRightBezierPoint(index, time, dx, dy[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param dx: :class:`float<PySide.QtCore.double>`
    :param dy: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Moves the right Bezier point at the given *index* (zero-based) by the given delta (dx,dy).
The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.




.. method:: NatronEngine.BezierCurve.removeControlPointByIndex(index[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param view: :class:`view<PySide.QtCore.QString>`

Removes the control point at the given *index* (zero-based).



.. method:: NatronEngine.BezierCurve.setCurveFinished(finished[, view = "All"])


    :param finished: :class:`bool<PySide.QtCore.bool>`
    :param view: :class:`view<PySide.QtCore.QString>`


Set whether the curve should be finished or not. See :func:`isCurveFinished()<NatronEngine.BezierCurve.isCurveFinished>`



.. method:: NatronEngine.BezierCurve.setFeatherPointAtIndex(index, time, x, y, lx, ly, rx, ry[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param lx: :class:`float<PySide.QtCore.double>`
    :param ly: :class:`float<PySide.QtCore.double>`
    :param rx: :class:`float<PySide.QtCore.double>`
    :param ry: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Set the feather point at the given *index* at  the position (x,y) with the left Bezier point
at (lx,ly) and right Bezier point at (rx,ry).

The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.



.. method:: NatronEngine.BezierCurve.setPointAtIndex(index, time, x, y, lx, ly, rx, ry[, view = "All"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`
    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param lx: :class:`float<PySide.QtCore.double>`
    :param ly: :class:`float<PySide.QtCore.double>`
    :param rx: :class:`float<PySide.QtCore.double>`
    :param ry: :class:`float<PySide.QtCore.double>`
    :param view: :class:`view<PySide.QtCore.QString>`

Set the point at the given *index* at  the position (x,y) with the left Bezier point
at (lx,ly) and right Bezier point at (rx,ry).

The *time* parameter is given so that if auto-keying is enabled a new keyframe will be set.

.. method:: NatronEngine.BezierCurve.splitView (view)

    :param view: :class:`view<PySide.QtCore.QString>`

Split-off the given *view* in the Bezier so that it can be assigned different shape
and animation than the *Main* view.
See :ref:`the section on multi-view<multiViewParams>` for more infos.

.. method:: NatronEngine.BezierCurve.unSplitView (view)

    :param view: :class:`view<PySide.QtCore.QString>`

If the given *view* was previously split off by a call to :func:`splitView(view)<NatronEngine.BezierCurve.splitView>`
then the view-specific values and animation will be removed and all subsequent access
to these values will return the value of the *Main* view.
See :ref:`the section on multi-view<multiViewParams>` for more infos.


.. method:: NatronEngine.BezierCurve.getViewsList ()

    :rtype: :class:`Sequence`

Returns a list of all views that have a different shape in the Bezier. All views
of the project that do not appear in this list are considered to be the same as
the first view returned by this function.
