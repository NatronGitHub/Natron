.. currentmodule:: NatronEngine
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

.. method:: Int2DParam.get()


    :rtype: :class: `Int2DTuple`

Returns a :doc:`Int2DTuple` containing the [x,y] value of this parameter at the timeline's
current time.


.. method:: Int2DParam.get(frame)
    :noindex:

     :param: :class:`float`
     :rtype: :class: `Int2DTuple`

Returns a :doc:`Int2DTuple` containing the [x,y] value of this parameter at
the given *frame*.


.. method:: Int2DParam.set(x, y)


    :param x: :class:`int`
    :param y: :class:`int`

Same as :func:`set(x)<NatronEngine.IntParam.set>` but for 2-dimensional integers.



.. method:: Int2DParam.set(x, y, frame)
    :noindex:


    :param x: :class:`int`
    :param y: :class:`int`
    :param frame: :class:`float`

Same as :func:`set(x,frame)<NatronEngine.IntParam.set>` but for 2-dimensional integers.






