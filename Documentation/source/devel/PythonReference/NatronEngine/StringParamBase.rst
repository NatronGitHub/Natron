.. module:: NatronEngine
.. _StringParamBase:

StringParamBase
***************

**Inherits** :doc:`AnimatedParam`

**Inherited by:** :ref:`PathParam`, :ref:`OutputFileParam`, :ref:`FileParam`, :ref:`StringParam`

Synopsis
--------

This is the base-class for all parameters holding a string.
See :ref:`here<stringBaseDetails>` for more details.

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.StringParamBase.get>` ([view="Main"])
- def :meth:`get<NatronEngine.StringParamBase.get>` (frame[,view="Main"])
- def :meth:`getDefaultValue<NatronEngine.StringParamBase.getDefaultValue>` ()
- def :meth:`getValue<NatronEngine.StringParamBase.getValue>` ([view="Main"])
- def :meth:`getValueAtTime<NatronEngine.StringParamBase.getValueAtTime>` (time[,view="Main"])
- def :meth:`restoreDefaultValue<NatronEngine.StringParamBase.restoreDefaultValue>` ([view="All"])
- def :meth:`set<NatronEngine.StringParamBase.set>` (x[,view="All"])
- def :meth:`set<NatronEngine.StringParamBase.set>` (x, frame[,view="All"])
- def :meth:`setDefaultValue<NatronEngine.StringParamBase.setDefaultValue>` (value)
- def :meth:`setValue<NatronEngine.StringParamBase.setValue>` (value[,view="All"])
- def :meth:`setValueAtTime<NatronEngine.StringParamBase.setValueAtTime>` (value, time[,view="All")

.. _stringBaseDetails:

Detailed Description
--------------------

A string parameter contains internally a string which can change over time.
Much like keyframes for value parameters (like :doc:`IntParam` or :doc:`DoubleParam`)
keyframes can be set on string params, though the interpolation will remain constant
always.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.StringParamBase.get([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`str<NatronEngine.std::string>`


Get the value of the parameter at the current timeline's time for the given *view*




.. method:: NatronEngine.StringParamBase.get(frame[,view="Main"])


    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`str<NatronEngine.std::string>`


Get the value of the parameter at the given *frame* and *view*.




.. method:: NatronEngine.StringParamBase.getDefaultValue()


    :rtype: :class:`str<NatronEngine.std::string>`

Get the default value for this parameter.





.. method:: NatronEngine.StringParamBase.getValue([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`str<NatronEngine.std::string>`



Same as :func:`get()<NatronEngine.StringParamBase.get>`



.. method:: NatronEngine.StringParamBase.getValueAtTime(time[,view="Main"])


    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`str<NatronEngine.std::string>`

Same as :func:`get(frame)<NatronEngine.StringParamBase.get>`



.. method:: NatronEngine.StringParamBase.restoreDefaultValue([view="All"])


    :param view: :class:`str<PySide.QtCore.QString>`

Removes all animation and expression set on this parameter for the given *view*
and set the value to be the default value.




.. method:: NatronEngine.StringParamBase.set(x[, view="All"])


    :param x: :class:`str<NatronEngine.std::string>`
    :param view: :class:`str<PySide.QtCore.QString>`

Set the value of this parameter to be *x* for the given *view*.
If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.




.. method:: NatronEngine.StringParamBase.set(x, frame[, view="All"])


    :param x: :class:`str<NatronEngine.std::string>`
    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`


Set a new keyframe on the parameter with the value *x* at the given *frame* and *view*.




.. method:: NatronEngine.StringParamBase.setDefaultValue(value)


    :param value: :class:`str<NatronEngine.std::string>`

Set the default *value* for this parameter.




.. method:: NatronEngine.StringParamBase.setValue(value[,view="All"])


    :param value: :class:`str<NatronEngine.std::string>`
    :param view: :class:`str<PySide.QtCore.QString>`


Same as :func:`set<NatronEngine.StringParamBase.setValue>`




.. method:: NatronEngine.StringParamBase.setValueAtTime(value, time[,view="All"])


    :param value: :class:`str<NatronEngine.std::string>`
    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`



Same as :func:`set(time)<NatronEngine.StringParamBase.set`




