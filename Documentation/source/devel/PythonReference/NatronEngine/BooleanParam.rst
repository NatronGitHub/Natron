.. module:: NatronEngine
.. _BooleanParam:

BooleanParam
************

**Inherits** :doc:`AnimatedParam`

Synopsis
--------

A parameter that contains a boolean value. See :ref:`detailed<bparam-details>` description below

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.BooleanParam.get>` ([view="Main"])
- def :meth:`get<NatronEngine.BooleanParam.get>` (frame[,view="Main"])
- def :meth:`getDefaultValue<NatronEngine.BooleanParam.getDefaultValue>` ()
- def :meth:`getValue<NatronEngine.BooleanParam.getValue>` ([view="Main"])
- def :meth:`getValueAtTime<NatronEngine.BooleanParam.getValueAtTime>` (time[, view="Main"])
- def :meth:`restoreDefaultValue<NatronEngine.BooleanParam.restoreDefaultValue>` ([view="All"])
- def :meth:`set<NatronEngine.BooleanParam.set>` (x[,view="All"])
- def :meth:`set<NatronEngine.BooleanParam.set>` (x, frame[, view="All"])
- def :meth:`setDefaultValue<NatronEngine.BooleanParam.setDefaultValue>` (value)
- def :meth:`setValue<NatronEngine.BooleanParam.setValue>` (value[,view="All"])
- def :meth:`setValueAtTime<NatronEngine.BooleanParam.setValueAtTime>` (value, time[,view="All"])

.. _bparam-details:

Detailed Description
--------------------

A BooleanParam looks like a checkbox in the user interface.

.. figure:: booleanParam.png


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.BooleanParam.get([view=Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns the value of the parameter at the current timeline's time for the given *view*.




.. method:: NatronEngine.BooleanParam.get(frame[,view="Main"])


    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns the value of the parameter at the given *frame* and *view*. This value may be interpolated
given the *interpolation* of the underlying animation curve.



.. method:: NatronEngine.BooleanParam.getDefaultValue()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns the default value for this parameter.




.. method:: NatronEngine.BooleanParam.getValue([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`PySide.QtCore.bool`


Same as :func:`get(view)<NatronEngine.BooleanParam.get>`



.. method:: NatronEngine.BooleanParam.getValueAtTime(time[,view="Main"])


    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Same as :func:`get(frame,view)<NatronEngine.BooleanParam.get>`




.. method:: NatronEngine.BooleanParam.restoreDefaultValue([view="All"])

    :param view: :class:`str<PySide.QtCore.QString>`

Removes all animation and expression set on this parameter at the given *view* and set the value
to be the default value.





.. method:: NatronEngine.BooleanParam.set(x[,view="All"])


    :param x: :class:`bool<PySide.QtCore.bool>`
    :param view: :class:`str<PySide.QtCore.QString>`

Set the value of this parameter to be *x* for the given *view*. If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.



.. method:: NatronEngine.BooleanParam.set(x, frame[,view="All"])


    :param x: :class:`bool<PySide.QtCore.bool>`
    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`

Set a new keyframe on the parameter with the value *x* at the given *frame* and *view*.




.. method:: NatronEngine.BooleanParam.setDefaultValue(value)


    :param value: :class:`bool<PySide.QtCore.bool>`

Set the default *value* for this parameter.




.. method:: NatronEngine.BooleanParam.setValue(value[,view="All"])


    :param value: :class:`bool<PySide.QtCore.bool>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(value,view)<NatronEngine.BooleanParam.set>`




.. method:: NatronEngine.BooleanParam.setValueAtTime(value, time[,view="All"])


    :param value: :class:`bool<PySide.QtCore.bool>`
    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set(value,time, view)<NatronEngine.BooleanParam.set>`






