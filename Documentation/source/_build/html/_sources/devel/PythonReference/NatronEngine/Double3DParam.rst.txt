.. module:: NatronEngine
.. _Double3DParam:

Double3DParam
*************

**Inherits** :doc:`Double2DParam`

Synopsis
--------

See :doc:`DoubleParam` for more information on this class.

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.Double3DParam.get>` ()
- def :meth:`get<NatronEngine.Double3DParam.get>` (frame)
- def :meth:`set<NatronEngine.Double3DParam.set>` (x, y, z)
- def :meth:`set<NatronEngine.Double3DParam.set>` (x, y, z, frame)



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Double3DParam.get()

    :rtype: :class:`Double3DTuple`

Returns a :doc:`Double3DTuple` with the [x,y,z] values for this parameter at the current
timeline's time.



.. method:: NatronEngine.Double3DParam.get(frame)

    :param frame: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple`

Returns a :doc:`Double3DTuple` with the [x,y,z] values for this parameter at the given *frame*.





.. method:: NatronEngine.Double3DParam.set(x, y, z, frame)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param z: :class:`float<PySide.QtCore.double>`
    :param frame: :class:`PySide.QtCore.int`


Same as :func:`set(x,frame)<NatronEngine.DoubleParam.set>` but for 3-dimensional doubles.




.. method:: NatronEngine.Double3DParam.set(x, y, z)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param z: :class:`float<PySide.QtCore.double>`


Same as :func:`set(x)<NatronEngine.DoubleParam.set>` but for 3-dimensional doubles.





