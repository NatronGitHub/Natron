.. module:: NatronEngine
.. _Int2DParam:

Int2DParam
**********

**Inherits** :doc:`IntParam`

**Inherited by:** :doc:`Int3DParam`

Synopsis
--------

See :doc:`IntParam` for more details.

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.Int2DParam.set>` ()
- def :meth:`get<NatronEngine.Int2DParam.set>` (frame)
- def :meth:`set<NatronEngine.Int2DParam.set>` (x, y)
- def :meth:`set<NatronEngine.Int2DParam.set>` (x, y, frame)

Detailed Description
--------------------

.. method:: NatronEngine.Int2DParam.get()


    :rtype: :class: `Int2DTuple`

Returns a :doc:`Int2DTuple` containing the [x,y] value of this parameter at the timeline's
current time.


.. method:: NatronEngine.Int2DParam.get(frame)

     :param: :class:`float<PySide.QtCore.float>`
     :rtype: :class: `Int2DTuple`

Returns a :doc:`Int2DTuple` containing the [x,y] value of this parameter at
the given *frame*.


.. method:: NatronEngine.Int2DParam.set(x, y)


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`

Same as :func:`set(x)<NatronEngine.IntParam.set>` but for 2-dimensional integers.



.. method:: NatronEngine.Int2DParam.set(x, y, frame)


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`

Same as :func:`set(x,frame)<NatronEngine.IntParam.set>` but for 2-dimensional integers.






