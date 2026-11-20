.. currentmodule:: NatronEngine
.. _BooleanParam:

BooleanParam
************

**Inherits** :doc:`AnimatedParam`

Synopsis
--------

A parameter that contains a boolean value. See :ref:`detailed<bparam-details>` description below

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.BooleanParam.get>` ()
- def :meth:`get<NatronEngine.BooleanParam.get>` (frame)
- def :meth:`getDefaultValue<NatronEngine.BooleanParam.getDefaultValue>` ()
- def :meth:`getValue<NatronEngine.BooleanParam.getValue>` ()
- def :meth:`getValueAtTime<NatronEngine.BooleanParam.getValueAtTime>` (time)
- def :meth:`restoreDefaultValue<NatronEngine.BooleanParam.restoreDefaultValue>` ()
- def :meth:`set<NatronEngine.BooleanParam.set>` (x)
- def :meth:`set<NatronEngine.BooleanParam.set>` (x, frame)
- def :meth:`setDefaultValue<NatronEngine.BooleanParam.setDefaultValue>` (value)
- def :meth:`setValue<NatronEngine.BooleanParam.setValue>` (value)
- def :meth:`setValueAtTime<NatronEngine.BooleanParam.setValueAtTime>` (value, time)

.. _bparam-details:

Detailed Description
--------------------

A BooleanParam looks like a checkbox in the user interface.

.. figure:: booleanParam.png


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: BooleanParam.get()


    :rtype: :class:`bool`

Returns the value of the parameter at the current timeline's time.




.. method:: BooleanParam.get(frame)
    :noindex:


    :param frame: :class:`float`
    :rtype: :class:`bool`

Returns the value of the parameter at the given *frame*. This value may be interpolated
given the *interpolation* of the underlying animation curve.




.. method:: BooleanParam.getDefaultValue()


    :rtype: :class:`bool`

Returns the default value for this parameter.




.. method:: BooleanParam.getValue()


    :rtype: :class:`bool`


Same as :func:`get()<NatronEngine.BooleanParam.get>`



.. method:: BooleanParam.getValueAtTime(time)


    :param time: :class:`float`
    :rtype: :class:`bool`

Same as :func:`get(frame)<NatronEngine.BooleanParam.get>`




.. method:: BooleanParam.restoreDefaultValue()


Removes all animation and expression set on this parameter and set the value
to be the default value.





.. method:: BooleanParam.set(x)


    :param x: :class:`bool`

Set the value of this parameter to be *x*. If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.



.. method:: BooleanParam.set(x, frame)
    :noindex:


    :param x: :class:`bool`
    :param frame: :class:`float`

Set a new keyframe on the parameter with the value *x* at the given *frame*.




.. method:: BooleanParam.setDefaultValue(value)


    :param value: :class:`bool`

Set the default *value* for this parameter.




.. method:: BooleanParam.setValue(value)


    :param value: :class:`bool`

Same as :func:`set(value)<NatronEngine.BooleanParam.set>`




.. method:: BooleanParam.setValueAtTime(value, time)


    :param value: :class:`bool`
    :param time: :class:`float`

Same as :func:`set(value,time)<NatronEngine.BooleanParam.set>`






