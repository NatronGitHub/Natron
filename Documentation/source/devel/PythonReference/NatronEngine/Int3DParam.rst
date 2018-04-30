.. module:: NatronEngine
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

.. method:: NatronEngine.Int3DParam.get()


    :rtype: :class:`<Int3DTuple>`

Returns a :doc:`Int3DTuple` containing the [x,y,z] value of this parameter at the timeline's
current time.


.. method:: NatronEngine.Int3DParam.get(frame)

    :param frame: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`<Int3DTuple>`


Returns a :doc:`Int3DTuple` containing the [x,y,z] value of this parameter at the given *frame*

.. method:: NatronEngine.Int3DParam.set(x, y, z)


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param z: :class:`int<PySide.QtCore.int>`

Same as :func:`set(x)<NatronEngine.IntParam.set>` but for 3-dimensional integers.



.. method:: NatronEngine.Int3DParam.set(x, y, z, frame)


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param z: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`


Same as :func:`set(x,frame)<NatronEngine.DoubleParam.set>` but for 3-dimensional integers.





