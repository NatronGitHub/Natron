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

- def :meth:`get<NatronEngine.Int2DParam.get>` ([view="Main"])
- def :meth:`get<NatronEngine.Int2DParam.get>` (frame[,view="Main"])
- def :meth:`set<NatronEngine.Int2DParam.set>` (x, y[,view="All"])
- def :meth:`set<NatronEngine.Int2DParam.set>` (x, y, frame [,view="All"])

Detailed Description
--------------------

.. method:: NatronEngine.Int2DParam.get([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class: `Int2DTuple`

Returns a :doc:`Int2DTuple` containing the [x,y] value of this parameter at the timeline's
current time for the given *view*.


.. method:: NatronEngine.Int2DParam.get(frame[,view="Main"])

     :param frame: :class:`float<PySide.QtCore.float>`
     :param view: :class:`str<PySide.QtCore.QString>`
     :rtype: :class: `Int2DTuple`

Returns a :doc:`Int2DTuple` containing the [x,y] value of this parameter at
the given *frame* and *view*.


.. method:: NatronEngine.Int2DParam.set(x, y[,view="All"])


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(x, view)<NatronEngine.IntParam.set>` but for 2-dimensional integers.



.. method:: NatronEngine.Int2DParam.set(x, y, frame[, view="All"])


    :param x: :class:`int<PySide.QtCore.int>`
    :param y: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(x,frame, view)<NatronEngine.IntParam.set>` but for 2-dimensional integers.






