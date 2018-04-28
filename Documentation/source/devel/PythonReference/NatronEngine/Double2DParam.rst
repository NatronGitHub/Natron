.. module:: NatronEngine
.. _Double2DParam:

Double2DParam
*************

**Inherits** :doc:`DoubleParam`

**Inherited by:** :ref:`Double3DParam`

Synopsis
--------

See :doc:`DoubleParam` for more informations on this class.

Functions
^^^^^^^^^

*    def :meth:`setCanAutoFoldDimensions<NatronEngine.Double2DParam.setCanAutoFoldDimensions` (enabled)
*    def :meth:`get<NatronEngine.Double2DParam.get>` ([view="Main"])
*    def :meth:`get<NatronEngine.Double2DParam.get>` (frame[,view="Main"])
*    def :meth:`set<NatronEngine.Double2DParam.set>` (x, y[,view="All"])
*    def :meth:`set<NatronEngine.Double2DParam.set>` (x, y, frame[,view="All"])


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Double2DParam.setCanAutoFoldDimensions(enabled)

    :param enabled: :class:`bool`

Sets whether all dimensions should be presented as a single vakue/slider whenever they are equal.


.. method:: NatronEngine.Double2DParam.get([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`Double2DTuple`

Returns a :doc:`Double2DTuple` with the [x,y] values for this parameter at the current
timeline's time for the given *view*.



.. method:: NatronEngine.Double2DParam.get(frame[,view="Main"])

    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`Double2DTuple`

Returns a :doc:`Double2DTuple` with the [x,y] values for this parameter at the given *frame*
and *view*.



.. method:: NatronEngine.Double2DParam.set(x, y, frame[,view="All"])


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`


Same as :func:`set(x,frame, view)<NatronEngine.DoubleParam.set>` but for 2-dimensional doubles.



.. method:: NatronEngine.Double2DParam.set(x, y[,view="All"])


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(x,view)<NatronEngine.DoubleParam.set>` but for 2-dimensional doubles.





