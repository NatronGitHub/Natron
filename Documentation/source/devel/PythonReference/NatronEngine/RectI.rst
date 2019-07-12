.. module:: NatronEngine
.. _RectI:

RectI
******

Synopsis
--------

A rectangle defined with integer precision. See :ref:`detailed<rectiDtails>` description below

Functions
^^^^^^^^^

- def :meth:`bottom<NatronEngine.RectI.bottom>` ()
- def :meth:`clear<NatronEngine.RectI.clear>` ()
- def :meth:`contains<NatronEngine.RectI.contains>` (otherRect)
- def :meth:`height<NatronEngine.RectI.height>` ()
- def :meth:`intersect<NatronEngine.RectI.intersect>` (otherRect)
- def :meth:`intersects<NatronEngine.RectI.intersects>` (otherRect)
- def :meth:`isInfinite<NatronEngine.RectI.isInfinite>` ()
- def :meth:`isNull<NatronEngine.RectI.isNull>` ()
- def :meth:`left<NatronEngine.RectI.left>` ()
- def :meth:`merge<NatronEngine.RectI.merge>` (otherRect)
- def :meth:`right<NatronEngine.RectI.right>` ()
- def :meth:`set<NatronEngine.RectI.set>` (x1,y1,x2,y2)
- def :meth:`set_bottom<NatronEngine.RectI.set_bottom>` (y1)
- def :meth:`set_left<NatronEngine.RectI.set_left>` (x1)
- def :meth:`set_right<NatronEngine.RectI.set_right>` (x2)
- def :meth:`set_top<NatronEngine.RectI.set_top>` (y2)
- def :meth:`top<NatronEngine.RectI.top>` ()
- def :meth:`translate<NatronEngine.RectI.translate>` (dx,dy)
- def :meth:`width<NatronEngine.RectI.width>` ()

.. _rectiDtails:

Detailed Description
--------------------

A rectangle where x1 < x2 and y1 < y2 such as width() == (x2 - x1) && height() == (y2 - y1)
(x1,y1) is are the coordinates of the bottom left corner of the rectangle.
The last element valid in the y dimension is y2 - 1 and the last valid in the x dimension is x2 - 1.
x1,x2,y1 and y2 are with integer precision.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.RectI.bottom()

    :rtype: :class:`int<PySide.QtCore.int>`

Returns the bottom edge, that is the


.. method:: NatronEngine.RectI.clear()

Same as :func:`set<NatronEngine.RectI.set>` (0,0,0,0)

.. method:: NatronEngine.RectI.contains(otherRect)

    :param otherRect: :class:`RectI<NatronEngine.RectI>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if *otherRect* is contained in or equals this rectangle, that is if::

    otherRect.x1 >= x1 and
    otherRect.y1 >= y1 and
    otherRect.x2 <= x2 and
    otherRect.y2 <= y2


.. method:: NatronEngine.RectI.height()

    :rtype: :class:`int<PySide.QtCore.int>`

Returns the height of the rectangle, that is: y2 - y1


.. method:: NatronEngine.RectI.intersect(otherRect)

    :param otherRect: :class:`RectI<NatronEngine.RectI>`
    :rtype: :class:`RectI<NatronEngine.RectI>`

Returns the intersection between this rectangle and *otherRect*. If the intersection is empty,
the return value will have the :func:`isNull()<NatronEngine.Rect.isNull>` function return True.



.. method:: NatronEngine.RectI.intersects(otherRect)

    :param otherRect: :class:`RectI<NatronEngine.RectI>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if rectangle and *otherRect* intersect.


.. method:: NatronEngine.RectI.isInfinite()

    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if this rectangle is considered to cover an infinite area. Some generator
effects use this to indicate that they can potentially generate an image of infinite size.


.. method:: NatronEngine.RectI.isNull()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns true if x2 <= x1 or y2 <= y1



.. method:: NatronEngine.RectI.left()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns x1, that is the position of the left edge of the rectangle.




.. method:: NatronEngine.RectI.merge(otherRect)


    :param otherRect: :class:`RectI<NatronEngine.RectI>`

Unions this rectangle with *otherRect*. In other words, this rectangle becomes the bounding box of this rectangle and  *otherRect*.

.. method:: NatronEngine.RectI.left()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns x1, that is the position of the left edge of the rectangle.


.. method:: NatronEngine.RectI.right()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns x2, that is the position of the right edge of the rectangle. x2 is considered to be
the first element outside the rectangle.


.. method:: NatronEngine.RectI.set(x1,y1,x2,y2)

    :param x1: :class:`int<PySide.QtCore.int>`
    :param y1: :class:`int<PySide.QtCore.int>`
    :param x2: :class:`int<PySide.QtCore.int>`
    :param y2: :class:`int<PySide.QtCore.int>`

Set the x1, y1, x2, y2 coordinates of this rectangle.



.. method:: NatronEngine.RectI.set_bottom(y1)


    :param y1: :class:`int<PySide.QtCore.int>`

Set y1

.. method:: NatronEngine.RectI.set_left(x1)


    :param y1: :class:`int<PySide.QtCore.int>`

Set x1

.. method:: NatronEngine.RectI.set_right(x2)


    :param x2: :class:`int<PySide.QtCore.int>`

Set x2


.. method:: NatronEngine.RectI.set_top(y2)


    :param y2: :class:`int<PySide.QtCore.int>`

Set y2


.. method:: NatronEngine.RectI.top()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns y2, that is the position of the top edge of the rectangle.
y2 is considered to be the first element outside the rectangle.


.. method:: NatronEngine.RectI.translate(dx,dy)


    :param dx: :class:`int<PySide.QtCore.int>`
    :param dy: :class:`int<PySide.QtCore.int>`

Moves all edges of the rectangle by *dx*, *dy*, that is::

        x1 += dx;
        y1 += dy;
        x2 += dx;
        y2 += dy;


.. method:: NatronEngine.RectI.width()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the width of the rectangle, that is x2 - x1.




