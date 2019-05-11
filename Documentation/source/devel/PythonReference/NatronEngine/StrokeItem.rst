.. module:: NatronEngine
.. _StrokeItem:

StrokeItem
**********

**Inherits** :doc:`ItemBase`

Synopsis
--------

A small item sub-class that handles strokes painted by the user

See :ref:`detailed<strokeItem.details>` description...

Functions
^^^^^^^^^

- def :meth:`getBoundingBox<NatronEngine.StrokeItem.getBoundingBox>` (time [, view="Main"])
- def :meth:`getBrushType<NatronEngine.StrokeItem.getBrushType>` ()
- def :meth:`getPoints<NatronEngine.StrokeItem.getPoints>` ()
- def :meth:`setPoints<NatronEngine.StrokeItem.setPoints>` (strokes)

.. _strokeItem.details:

Detailed Description
--------------------

A stroke is internally a list of 2D points with a timestamp and a pressure for each sample.
Each sample is of type :ref:`StrokePoint<StrokePoint>`.

You can get a list of all samples with the func:`getPoints()<NatronEngine.StrokeItem.getPoints>` function.
You can set the entire stroke points at once with the  func:`setPoints(strokes)<NatronEngine.StrokeItem.setPoints>` function.

A stroke item consists of multiple disconnected sub-strokes. For example if the user starts drawing, releases the pen
and then draws again without changing settings, it is likely the same :ref:`item<ItemBase>` will
be re-used instead of creating a new one.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: NatronEngine.StrokeItem.getBoundingBox(time [, view="Main"])


    :param time: :class:`double<PySide.QtCore.double>`
    :param view: :class:`str<PySide.QtCore.QString>`

Returns the bounding box of the stroke at the given *time* and *view*.
This takes into account any transform applied on the stroke.



.. method:: NatronEngine.StrokeItem.getBrushType()

    :rtype: :class:`RotoStrokeItem<NatronEngine.Natron.RotoStrokeItem>`


Returns the type of brush used by the stroke.



.. method:: NatronEngine.StrokeItem.getPoints()


    :rtype: :class:`Sequence`

Returns a list of all sub-strokes in the stroke. Each sub-stroke corresponds to one
different stroke and sub-strokes are not necessarily connected.
A sub-stroke is a list of :ref:`StrokePoint<StrokePoint>`.


.. method:: NatronEngine.StrokeItem.setPoints(subStrokes)


    :param subStrokes: :class:`Sequence`


Set the item sub-strokes from the given *subStrokes*.
Each sub-stroke corresponds to one
different stroke and sub-strokes are not necessarily connected.
A sub-stroke is a list of :ref:`StrokePoint<StrokePoint>`.




