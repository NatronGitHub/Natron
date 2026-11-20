.. currentmodule:: NatronEngine
.. _Int3DParam:

Int3DParam
**********

**Inherits** :doc:`Int2DParam`

Synopsis
--------

See :doc:`IntParam` for more details.

Functions
^^^^^^^^^

- def :meth:`set<NatronEngine.Int3DParam.get>` ()
- def :meth:`set<NatronEngine.Int3DParam.get>` (frame)
- def :meth:`set<NatronEngine.Int3DParam.set>` (x, y, z)
- def :meth:`set<NatronEngine.Int3DParam.set>` (x, y, z, frame)

Detailed Description
--------------------

.. method:: Int3DParam.get()


    :rtype: :class:`Int3DTuple<NatronEngine.Int3DTuple>`

Returns a :doc:`Int3DTuple` containing the [x,y,z] value of this parameter at the timeline's
current time.


.. method:: Int3DParam.get(frame)
    :noindex:

    :param frame: :class:`float`
    :rtype: :class:`Int3DTuple<NatronEngine.Int3DTuple>`


Returns a :doc:`Int3DTuple` containing the [x,y,z] value of this parameter at the given *frame*

.. method:: Int3DParam.set(x, y, z)


    :param x: :class:`int`
    :param y: :class:`int`
    :param z: :class:`int`

Same as :func:`set(x)<NatronEngine.IntParam.set>` but for 3-dimensional integers.



.. method:: Int3DParam.set(x, y, z, frame)
    :noindex:


    :param x: :class:`int`
    :param y: :class:`int`
    :param z: :class:`int`
    :param frame: :class:`float`


Same as :func:`set(x,frame)<NatronEngine.DoubleParam.set>` but for 3-dimensional integers.





