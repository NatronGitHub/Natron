.. module:: NatronEngine
.. _Double3DParam:

Double3DParam
*************

**Inherits** :doc:`Double2DParam`

Synopsis
--------

See :doc:`DoubleParam` for more informations on this class.

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.Double3DParam.get>` ([view="Main"])
- def :meth:`get<NatronEngine.Double3DParam.get>` (frame[,view="Main"])
- def :meth:`set<NatronEngine.Double3DParam.set>` (x, y, z[,view="All"])
- def :meth:`set<NatronEngine.Double3DParam.set>` (x, y, z, frame[,view="All"])

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Double3DParam.get([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`Double3DTuple`

Returns a :doc:`Double3DTuple` with the [x,y,z] values for this parameter at the current
timeline's time for the given *view*.



.. method:: NatronEngine.Double3DParam.get(frame[,view="Main"])

    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`Double3DTuple`

Returns a :doc:`Double3DTuple` with the \[x,y,z\] values for this parameter at the given *frame*
and *view*.





.. method:: NatronEngine.Double3DParam.set(x, y, z, frame [, view="All"])


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param z: :class:`float<PySide.QtCore.double>`
    :param frame: :class:`PySide.QtCore.int`
    :param view: :class:`str<PySide.QtCore.QString>`


Same as :func:`set(x,frame, view)<NatronEngine.DoubleParam.set>` but for 3-dimensional doubles.




.. method:: NatronEngine.Double3DParam.set(x, y, z[, view = "All"])


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param z: :class:`float<PySide.QtCore.double>`
    :param view: :class:`str<PySide.QtCore.QString>`


Same as :func:`set(x, view)<NatronEngine.DoubleParam.set>` but for 3-dimensional doubles.





