.. module:: NatronEngine
.. _IntParam:

IntParam
********

*Inherits* :doc:`AnimatedParam`

**Inherited by:** :doc:`Int2DParam`, :doc:`Int3DParam`

Synopsis
--------

An IntParam can contain one or multiple int values.
See :ref:`detailed<int.details>` description...

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.IntParam.get>` ()
- def :meth:`get<NatronEngine.IntParam.get>` (frame)
- def :meth:`getDefaultValue<NatronEngine.IntParam.getDefaultValue>` ([dimension=0])
- def :meth:`getDisplayMaximum<NatronEngine.IntParam.getDisplayMaximum>` (dimension)
- def :meth:`getDisplayMinimum<NatronEngine.IntParam.getDisplayMinimum>` (dimension)
- def :meth:`getMaximum<NatronEngine.IntParam.getMaximum>` ([dimension=0])
- def :meth:`getMinimum<NatronEngine.IntParam.getMinimum>` ([dimension=0])
- def :meth:`getValue<NatronEngine.IntParam.getValue>` ([dimension=0])
- def :meth:`getValueAtTime<NatronEngine.IntParam.getValueAtTime>` (time[, dimension=0])
- def :meth:`restoreDefaultValue<NatronEngine.IntParam.restoreDefaultValue>` ([dimension=0])
- def :meth:`set<NatronEngine.IntParam.set>` (x)
- def :meth:`set<NatronEngine.IntParam.set>` (x, frame)
- def :meth:`setDefaultValue<NatronEngine.IntParam.setDefaultValue>` (value[, dimension=0])
- def :meth:`setDisplayMaximum<NatronEngine.IntParam.setDisplayMaximum>` (maximum[, dimension=0])
- def :meth:`setDisplayMinimum<NatronEngine.IntParam.setDisplayMinimum>` (minimum[, dimension=0])
- def :meth:`setMaximum<NatronEngine.IntParam.setMaximum>` (maximum[, dimension=0])
- def :meth:`setMinimum<NatronEngine.IntParam.setMinimum>` (minimum[, dimension=0])
- def :meth:`setValue<NatronEngine.IntParam.setValue>` (value[, dimension=0])
- def :meth:`setValueAtTime<NatronEngine.IntParam.setValueAtTime>` (value, time[, dimension=0])

.. _int.details:

Detailed Description
--------------------


An int param can have 1 to 3 dimensions. (See :doc:`Int2DParam` and :doc:`Int3DParam`).
Usually this is used to represent a single integer value that may animate over time.

The user interface for them varies depending on the number of dimensions.
*Screenshots are the same than for the :doc`DoubleParam` because the user interface is the same*

A 1-dimensional :doc:`IntParam`

.. figure:: doubleParam.png

A 2-dimensional :doc:`Int2DParam`

.. figure:: double2DParam.png

A 3-dimensional :doc:`Int3DParam`

.. figure:: double3DParam.png

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: NatronEngine.IntParam.get(frame)


    :param frame: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`int<PySide.QtCore.int>`


Returns the value of this parameter at the given *frame*. If the animation curve has an
animation (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>` then the
value will be interpolated using the *interpolation* chosen by the user for the curve.



.. method:: NatronEngine.IntParam.get()


    :rtype: :class:`int<PySide.QtCore.int>`


Returns the value of this parameter at the given current timeline's time.




.. method:: NatronEngine.IntParam.getDefaultValue([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`


Returns the default value for this parameter. *dimension* is meaningless for the IntParam
class because it is 1-dimensional, but is useful for inherited classes :doc:`Int2DParam`
and :doc:`Int3DParam`



.. method:: NatronEngine.IntParam.getDisplayMaximum(dimension)


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`


Returns the display maximum for this parameter at the given *dimension*.
The display maximum is the maximum value visible on the slider, internally the value
can exceed this range.



.. method:: NatronEngine.IntParam.getDisplayMinimum(dimension)


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the display minimum for this parameter at the given *dimension*.
The display minimum is the minimum value visible on the slider, internally the value
can exceed this range.





.. method:: NatronEngine.IntParam.getMaximum([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the maximum for this parameter at the given *dimension*.
The maximum value cannot be exceeded and any higher value will be clamped to this value.





.. method:: NatronEngine.IntParam.getMinimum([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`


Returns the minimum for this parameter at the given *dimension*.
The minimum value cannot be exceeded and any lower value will be clamped to this value.




.. method:: NatronEngine.IntParam.getValue([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`


Returns the value of this parameter at the given *dimension* at the current timeline's time.




.. method:: NatronEngine.IntParam.getValueAtTime(time[, dimension=0])


    :param time: :class:`float<PySide.QtCore.float>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`


Returns the value of this parameter at the given *dimension* at the given *time*.

If the animation curve has an
animation (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>` then the
value will be interpolated using the *interpolation* chosen by the user for the curve.




.. method:: NatronEngine.IntParam.restoreDefaultValue([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`


Returns the value of this parameter at the given *dimension* at the given *time*.




.. method:: NatronEngine.IntParam.set(x, frame)


    :param x: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`

Set a new keyframe on the parameter with the value *x* at the given *frame*.





.. method:: NatronEngine.IntParam.set(x)


    :param x: :class:`int<PySide.QtCore.int>`


Set the value of this parameter to be *x*.
If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.




.. method:: NatronEngine.IntParam.setDefaultValue(value[, dimension=0])


    :param value: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`

Set the default *value* for this parameter at the given *dimension*.





.. method:: NatronEngine.IntParam.setDisplayMaximum(maximum[, dimension=0])


    :param maximum: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`


Set the display maximum of the parameter to be *maximum* for the given *dimension*.
See :func:`getDisplayMaximum<NatronEngine.IntParam.getDisplayMaximum>`




.. method:: NatronEngine.IntParam.setDisplayMinimum(minimum[, dimension=0])


    :param minimum: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`



Set the display minimum of the parameter to be *minmum* for the given *dimension*.
See :func:`getDisplayMinimum<NatronEngine.IntParam.getDisplayMinimum>`




.. method:: NatronEngine.IntParam.setMaximum(maximum[, dimension=0])


    :param maximum: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`

Set the maximum of the parameter to be *maximum* for the given *dimension*.
See :func:`getMaximum<NatronEngine.IntParam.getMaximum>`






.. method:: NatronEngine.IntParam.setMinimum(minimum[, dimension=0])


    :param minimum: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`


Set the minimum of the parameter to be *minimum* for the given *dimension*.
See :func:`getMinimum<NatronEngine.IntParam.getMinimum>`





.. method:: NatronEngine.IntParam.setValue(value[, dimension=0])


    :param value: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`



Same as :func:`set(value,dimension)<NatronEngine.IntParam.set>`




.. method:: NatronEngine.IntParam.setValueAtTime(value, time[, dimension=0])


    :param value: :class:`int<PySide.QtCore.int>`
    :param time: :class:`float<PySide.QtCore.float>`
    :param dimension: :class:`int<PySide.QtCore.int>`


Same as :func:`set(value,time,dimension)<NatronEngine.IntParam.set>`





