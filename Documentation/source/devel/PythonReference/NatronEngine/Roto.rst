.. module:: NatronEngine
.. _Roto:

Roto
****

*Inherits* :ref:`ItemsTable<ItemsTable>`


Synopsis
--------

A derived class of :ref:`ItemsTable<ItemsTable>` that allows creating RotoPaint specific
items.

Functions
^^^^^^^^^

*    def :meth:`createBezier<NatronEngine.Roto.createBezier>` (x, y, time)
*    def :meth:`createEllipse<NatronEngine.Roto.createEllipse>` (x, y, diameter, fromCenter, time)
*    def :meth:`createLayer<NatronEngine.Roto.createLayer>` ()
*    def :meth:`createRectangle<NatronEngine.Roto.createRectangle>` (x, y, size, time)
*    def :meth:`createStroke<NatronEngine.Roto.createStroke>` (type)


.. _roto.details:

Detailed Description
--------------------

This class just overloads the :ref:`ItemsTable<ItemsTable>` class to add methods to create
items.
For more informations read the description of the  :ref:`ItemsTable<ItemsTable>` class.



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: NatronEngine.Roto.createBezier(x, y, time)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`BezierCurve<NatronEngine.BezierCurve>`

Creates a new :doc:`BezierCurve` with one control point at position (x,y) and a keyframe
at the given *time*.




.. method:: NatronEngine.Roto.createEllipse(x, y, diameter, fromCenter, time)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param diameter: :class:`float<PySide.QtCore.double>`
    :param fromCenter: :class:`bool<PySide.QtCore.bool>`
    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`BezierCurve<NatronEngine.BezierCurve>`

Creates a new ellipse. This is a convenience function that uses :func:`createBezier(x,y,time)<NatronEngine.Roto.createBezier>`
to create a new :doc:`BezierCurve` and then adds 3 other control points to the Bezier so that it forms an
ellipse of the given *diameter*. A new keyframe will be set at the given *time*.
If *fromCenter* is true, then (x,y) is understood to be the coordinates of the center of the ellipse,
otherwise (x,y) is understood to be the position of the top-left point of the smallest enclosing
rectangle of the ellipse.




.. method:: NatronEngine.Roto.createLayer()


    :rtype: :class:`Layer<NatronEngine.Layer>`

Creates a new layer.




.. method:: NatronEngine.Roto.createRectangle(x, y, size, time)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param size: :class:`float<PySide.QtCore.double>`
    :param time: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`BezierCurve<NatronEngine.BezierCurve>`


Creates a new rectangle. This is a convenience function that uses :func:`createBezier(x,y,time)<NatronEngine.Roto.createBezier>`
to create a new :doc:`BezierCurve` and then adds 3 other control points to the Bezier so that it forms a
rectangle of the given *size* on each of its sides. A new keyframe will be set at the given *time*.



.. method:: NatronEngine.Roto.getBaseLayer()


    :rtype: :class:`Layer<NatronEngine.Layer>`

Convenience function to access to the base :doc:`Layer`. Note that all shapes should belong
to a :doc:`Layer`, the base layer being the top-level parent of all the hierarchy.




.. method:: NatronEngine.Roto.getItemByName(name)


    :param name: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`ItemBase<NatronEngine.ItemBase>`

Returns an item by its *script-name*. See :ref:`this section<autoVar>` for the details of what is the
*script-name* of an item. E.g::

    app1.Roto1.roto.Layer1.Bezier1 = app1.Roto1.roto.getItemByName("Bezier1")


.. method:: NatronEngine.Roto.createStroke(type)

    :param type: :class:`RotoStrokeType<NatronEngine.Natron.RotoStrokeType>`
    :rtype: :class:`StrokeItem<NatronEngine.StrokeItem>`


Creates a new empty stroke item of the given type.

