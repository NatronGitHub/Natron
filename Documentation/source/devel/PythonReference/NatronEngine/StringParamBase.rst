.. module:: NatronEngine
.. _StringParamBase:

StringParamBase
***************

**Inherits** :doc:`AnimatedParam`

**Inherited by:** :doc:`PathParam`, :doc:`OutputFileParam`, :doc:`FileParam`, :doc:`StringParam`

Synopsis
--------

This is the base-class for all parameters holding a string.
See :ref:`here<stringBaseDetails>` for more details.

Functions
^^^^^^^^^

- def :meth:`get<NatronEngine.StringParamBase.get>` ()
- def :meth:`get<NatronEngine.StringParamBase.get>` (frame)
- def :meth:`getDefaultValue<NatronEngine.StringParamBase.getDefaultValue>` ()
- def :meth:`getValue<NatronEngine.StringParamBase.getValue>` ()
- def :meth:`getValueAtTime<NatronEngine.StringParamBase.getValueAtTime>` (time)
- def :meth:`restoreDefaultValue<NatronEngine.StringParamBase.restoreDefaultValue>` ()
- def :meth:`set<NatronEngine.StringParamBase.set>` (x)
- def :meth:`set<NatronEngine.StringParamBase.set>` (x, frame)
- def :meth:`setDefaultValue<NatronEngine.StringParamBase.setDefaultValue>` (value)
- def :meth:`setValue<NatronEngine.StringParamBase.setValue>` (value)
- def :meth:`setValueAtTime<NatronEngine.StringParamBase.setValueAtTime>` (value, time)

.. _stringBaseDetails:

Detailed Description
--------------------

A string parameter contains internally a string which can change over time. 
Much like keyframes for value parameters (like :doc:`IntParam` or :doc:`DoubleParam`) 
keyframes can be set on string params, though the interpolation will remain constant
always.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^




.. method:: NatronEngine.StringParamBase.get()


    :rtype: :class:`str<NatronEngine.std::string>`


Get the value of the parameter at the current timeline's time




.. method:: NatronEngine.StringParamBase.get(frame)


    :param frame: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`str<NatronEngine.std::string>`


Get the value of the parameter at the given *frame*.




.. method:: NatronEngine.StringParamBase.getDefaultValue()


    :rtype: :class:`str<NatronEngine.std::string>`

Get the default value for this parameter.





.. method:: NatronEngine.StringParamBase.getValue()


    :rtype: :class:`str<NatronEngine.std::string>`



Same as :func:`get()<NatronEngine.StringParamBase.get>`



.. method:: NatronEngine.StringParamBase.getValueAtTime(time)


    :param time: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`str<NatronEngine.std::string>`

Same as :func:`get(frame)<NatronEngine.StringParamBase.get>`



.. method:: NatronEngine.StringParamBase.restoreDefaultValue()



Removes all animation and expression set on this parameter and set the value
to be the default value.




.. method:: NatronEngine.StringParamBase.set(x)


    :param x: :class:`str<NatronEngine.std::string>`

Set the value of this parameter to be *x*. If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.




.. method:: NatronEngine.StringParamBase.set(x, frame)


    :param x: :class:`str<NatronEngine.std::string>`
    :param frame: :class:`float<PySide.QtCore.float>`


Set a new keyframe on the parameter with the value *x* at the given *frame*.




.. method:: NatronEngine.StringParamBase.setDefaultValue(value)


    :param value: :class:`str<NatronEngine.std::string>`

Set the default *value* for this parameter.




.. method:: NatronEngine.StringParamBase.setValue(value)


    :param value: :class:`str<NatronEngine.std::string>`


Same as :func:`set<NatronEngine.StringParamBase.setValue>`




.. method:: NatronEngine.StringParamBase.setValueAtTime(value, time)


    :param value: :class:`str<NatronEngine.std::string>`
    :param time: :class:`float<PySide.QtCore.float>`



Same as :func:`set(time)<NatronEngine.StringParamBase.set`




