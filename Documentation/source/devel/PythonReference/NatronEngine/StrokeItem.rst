.. currentmodule:: NatronEngine
.. _StrokeItem:

StrokeItem
**********

**Inherits** :doc:`ItemBase`

Synopsis
--------

A StrokeItem is a paint stroke of a RotoPaint node, created with
:meth:`Roto.createStroke()<NatronEngine.Roto.createStroke>`.
See :ref:`detailed<strokeItem.details>` description below.

Functions
^^^^^^^^^

- def :meth:`addStroke<NatronEngine.StrokeItem.addStroke>` (points)
- def :meth:`getBrushType<NatronEngine.StrokeItem.getBrushType>` ()
- def :meth:`getNumStrokes<NatronEngine.StrokeItem.getNumStrokes>` ()
- def :meth:`getPoints<NatronEngine.StrokeItem.getPoints>` (index)

.. _strokeItem.details:

Detailed Description
--------------------

A paint stroke is made of one or more lines, each drawn without lifting the pen, with the
parameters of the stroke (brush size, color, opacity...), which
:meth:`getParam()<NatronEngine.ItemBase.getParam>` returns. Each line is a list of points
``(x, y, pressure, timestamp)``: the position in canonical coordinates, the pen pressure (0 to 1)
and the time in seconds since the first point of the line, which the brush follows between the
points.

.. versionadded:: 2.6

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: StrokeItem.addStroke(points)

    :param points: :class:`sequence`
    :rtype: :class:`bool`

Adds a line made of *points*, a sequence of ``(x, y, pressure, timestamp)`` sequences, with
increasing timestamps. The line is interpolated between the points as a line painted in the viewer.
Returns False, and adds nothing, if *points* is empty or a point doesn't have 4 items.

.. method:: StrokeItem.getBrushType()

    :rtype: :attr:`NatronEngine.Natron.RotoStrokeType`

Returns the brush of the stroke, e.g. ``NatronEngine.Natron.RotoStrokeType.eRotoStrokeTypeSolid``.

.. method:: StrokeItem.getNumStrokes()

    :rtype: :class:`int`

Returns the number of lines of the stroke.

.. method:: StrokeItem.getPoints(index)

    :param index: :class:`int`
    :rtype: :class:`sequence`

Returns the points ``(x, y, pressure, timestamp)`` of the line at *index* (from 0 to
:meth:`getNumStrokes()<NatronEngine.StrokeItem.getNumStrokes>` - 1), or an empty list if there is
no such line.
