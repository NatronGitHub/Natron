.. module:: NatronEngine
.. _RectD:

RectD
******

Synopsis
--------

A rectangle defined with floating point precision. See :ref:`detailed<rectddDtails>` description below

Functions
^^^^^^^^^

- def :meth:`area<NatronEngine.RectD.area>` ()
- def :meth:`bottom<NatronEngine.RectD.bottom>` ()
- def :meth:`clear<NatronEngine.RectD.clear>` ()
- def :meth:`contains<NatronEngine.RectD.contains>` (otherRect)
- def :meth:`height<NatronEngine.RectD.height>` ()
- def :meth:`intersect<NatronEngine.RectD.intersect>` (otherRect)
- def :meth:`intersects<NatronEngine.RectD.intersects>` (otherRect)
- def :meth:`isInfinite<NatronEngine.RectD.isInfinite>` ()
- def :meth:`isNull<NatronEngine.RectD.isNull>` ()
- def :meth:`left<NatronEngine.RectD.left>` ()
- def :meth:`merge<NatronEngine.RectD.merge>` (otherRect)
- def :meth:`right<NatronEngine.RectD.right>` ()
- def :meth:`set<NatronEngine.RectD.set>` (x1,y1,x2,y2)
- def :meth:`set_bottom<NatronEngine.RectD.set_bottom>` (y1)
- def :meth:`set_left<NatronEngine.RectD.set_left>` (x1)
- def :meth:`set_right<NatronEngine.RectD.set_right>` (x2)
- def :meth:`set_top<NatronEngine.RectD.set_top>` (y2)
- def :meth:`top<NatronEngine.RectD.top>` ()
- def :meth:`translate<NatronEngine.RectD.translate>` (dx,dy)
- def :meth:`width<NatronEngine.RectD.width>` ()

.. _rectddDtails:

Detailed Description
--------------------

A rectangle where x1 < x2 and y1 < y2 such as width() == (x2 - x1) && height() == (y2 - y1)
(x1,y1) is are the coordinates of the bottom left corner of the rectangle.
The last element valid in the y dimension is y2 - 1 and the last valid in the x dimension is x2 - 1.
x1,x2,y1 and y2 are with floating point precision.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.RectD.area()


    :rtype: :class:`double<PySide.QtCore.double>`

Returns the area covered by the rectangle, that is: (y2 - y1) * (x2 - x1)


.. method:: NatronEngine.RectD.bottom()

    :rtype: :class:`double<PySide.QtCore.double>`

Returns the bottom edge, that is the


.. method:: NatronEngine.RectD.clear()

Same as :func:`set<NatronEngine.RectD.set>` (0,0,0,0)

.. method:: NatronEngine.RectD.contains(otherRect)

    :param otherRect: :class:`RectD<NatronEngine.RectD>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if *otherRect* is contained in or equals this rectangle, that is if::

    otherRect.x1 >= x1 and
    otherRect.y1 >= y1 and
    otherRect.x2 <= x2 and
    otherRect.y2 <= y2


.. method:: NatronEngine.RectD.height()

    :rtype: :class:`double<PySide.QtCore.double>`

Returns the height of the rectangle, that is: y2 - y1


.. method:: NatronEngine.RectD.intersect(otherRect)

    :param otherRect: :class:`RectD<NatronEngine.RectD>`
    :rtype: :class:`RectD<NatronEngine.RectD>`

Returns the intersection between this rectangle and *otherRect*. If the intersection is empty,
the return value will have the :func:`isNull()<NatronEngine.Rect.isNull>` function return True.



.. method:: NatronEngine.RectD.intersects(otherRect)

    :param otherRect: :class:`RectD<NatronEngine.RectD>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if rectangle and *otherRect* intersect.


.. method:: NatronEngine.RectD.isInfinite()

    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns True if this rectangle is considered to cover an infinite area. Some generator
effects use this to indicate that they can potentially generate an image of infinite size.


.. method:: NatronEngine.RectD.isNull()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns true if x2 <= x1 or y2 <= y1



.. method:: NatronEngine.RectD.left()


    :rtype: :class:`double<PySide.QtCore.double>`

Returns x1, that is the position of the left edge of the rectangle.




.. method:: NatronEngine.RectD.merge(otherRect)


    :param otherRect: :class:`RectD<NatronEngine.RectD>`

Unions this rectangle with *otherRect*. In other words, this rectangle becomes the bounding box of this rectangle and  *otherRect*.

.. method:: NatronEngine.RectD.left()


    :rtype: :class:`double<PySide.QtCore.double>`

Returns x1, that is the position of the left edge of the rectangle.


.. method:: NatronEngine.RectD.right()


    :rtype: :class:`double<PySide.QtCore.double>`

Returns x2, that is the position of the right edge of the rectangle. x2 is considered to be
the first element outside the rectangle.


.. method:: NatronEngine.RectD.set(x1,y1,x2,y2)

    :param x1: :class:`double<PySide.QtCore.double>`
    :param y1: :class:`double<PySide.QtCore.double>`
    :param x2: :class:`double<PySide.QtCore.double>`
    :param y2: :class:`double<PySide.QtCore.double>`

Set the x1, y1, x2, y2 coordinates of this rectangle.



.. method:: NatronEngine.RectD.set_bottom(y1)


    :param y1: :class:`double<PySide.QtCore.double>`

Set y1

.. method:: NatronEngine.RectD.set_left(x1)


    :param y1: :class:`double<PySide.QtCore.double>`

Set x1

.. method:: NatronEngine.RectD.set_right(x2)


    :param x2: :class:`double<PySide.QtCore.double>`

Set x2


.. method:: NatronEngine.RectD.set_top(y2)


    :param y2: :class:`double<PySide.QtCore.double>`

Set y2


.. method:: NatronEngine.RectD.top()


    :rtype: :class:`double<PySide.QtCore.double>`

Returns y2, that is the position of the top edge of the rectangle.
y2 is considered to be the first element outside the rectangle.


.. method:: NatronEngine.RectD.translate(dx,dy)


    :param dx: :class:`double<PySide.QtCore.double>`
    :param dy: :class:`double<PySide.QtCore.double>`

Moves all edges of the rectangle by *dx*, *dy*, that is::

        x1 += dx;
        y1 += dy;
        x2 += dx;
        y2 += dy;


.. method:: NatronEngine.RectD.width()


    :rtype: :class:`double<PySide.QtCore.double>`

Returns the width of the rectangle, that is x2 - x1.




