.. module:: NatronEngine
.. _Roto:

Roto
****


Synopsis
--------

This class encapsulates all things related to the roto node.
See detailed :ref:`description<roto.details>` below.

Functions
^^^^^^^^^

- def :meth:`createBezier<NatronEngine.Roto.createBezier>` (x, y, time)
- def :meth:`createEllipse<NatronEngine.Roto.createEllipse>` (x, y, diameter, fromCenter, time)
- def :meth:`createLayer<NatronEngine.Roto.createLayer>` ()
- def :meth:`createRectangle<NatronEngine.Roto.createRectangle>` (x, y, size, time)
- def :meth:`getBaseLayer<NatronEngine.Roto.getBaseLayer>` ()
- def :meth:`getItemByName<NatronEngine.Roto.getItemByName>` (name)

.. _roto.details:

Detailed Description
--------------------

The Roto class is uses for now in Natron exclusively by the roto node, but its functionalities
could be re-used for other nodes as well.
Its purpose is to manage all layers and shapes.
You can create new shapes with the :func:`createBezier(x,y,time)<NatronEngine.Roto.createBezier>`,
:func:`createEllipse(x,y,diameter,fromCenter,time)<NatronEngine.Roto.createEllipse>` and
:func:`createRectangle(x,y,size,time)<NatronEngine.Roto.createRectangle>` functions.

To create a new :doc:`Layer` you can use the :func:`createLayer()<NatronEngine.Roto.createLayer>` function.

As for other :ref:`auto-declared<autoVar>` variables, all shapes in the Roto objects can be
accessed by their script-name, e.g.:

    Roto1.roto.Layer1.Bezier1



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






