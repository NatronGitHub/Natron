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

- def :meth:`set<NatronEngine.Int3DParam.get>` ([view="Main"])
- def :meth:`set<NatronEngine.Int3DParam.get>` (frame[,view="Main"])
- def :meth:`set<NatronEngine.Int3DParam.set>` (x, y, z[,view="All"])
- def :meth:`set<NatronEngine.Int3DParam.set>` (x, y, z, frame[,view="All"])

Detailed Description
--------------------

.. method:: NatronEngine.Int3DParam.get([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`<Int3DTuple>`

Returns a :doc:`Int3DTuple` containing the [x,y,z] value of this parameter at the timeline's
current time for the given *view*.


.. method:: NatronEngine.Int3DParam.get(frame[,view="Main"])

    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`<Int3DTuple>`


Returns a :doc:`Int3DTuple` containing the [x,y,z] value of this parameter at the given *frame*
and *view*.

.. method:: NatronEngine.Int3DParam.set(x, y, z[,view="All"])


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param z: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(x, view)<NatronEngine.IntParam.set>` but for 3-dimensional integers.



.. method:: NatronEngine.Int3DParam.set(x, y, z, frame[,view="All"])


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param z: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(x,frame, view)<NatronEngine.DoubleParam.set>` but for 3-dimensional integers.





