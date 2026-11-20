.. currentmodule:: NatronEngine
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




.. method:: StringParamBase.get()


    :rtype: :class:`str`


Get the value of the parameter at the current timeline's time




.. method:: StringParamBase.get(frame)
    :noindex:


    :param frame: :class:`float`
    :rtype: :class:`str`


Get the value of the parameter at the given *frame*.




.. method:: StringParamBase.getDefaultValue()


    :rtype: :class:`str`

Get the default value for this parameter.





.. method:: StringParamBase.getValue()


    :rtype: :class:`str`



Same as :func:`get()<NatronEngine.StringParamBase.get>`



.. method:: StringParamBase.getValueAtTime(time)


    :param time: :class:`float`
    :rtype: :class:`str`

Same as :func:`get(frame)<NatronEngine.StringParamBase.get>`



.. method:: StringParamBase.restoreDefaultValue()



Removes all animation and expression set on this parameter and set the value
to be the default value.




.. method:: StringParamBase.set(x)


    :param x: :class:`str`

Set the value of this parameter to be *x*. If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.




.. method:: StringParamBase.set(x, frame)
    :noindex:


    :param x: :class:`str`
    :param frame: :class:`float`


Set a new keyframe on the parameter with the value *x* at the given *frame*.




.. method:: StringParamBase.setDefaultValue(value)


    :param value: :class:`str`

Set the default *value* for this parameter.




.. method:: StringParamBase.setValue(value)


    :param value: :class:`str`


Same as :func:`set<NatronEngine.StringParamBase.setValue>`




.. method:: StringParamBase.setValueAtTime(value, time)


    :param value: :class:`str`
    :param time: :class:`float`



Same as :func:`set(time)<NatronEngine.StringParamBase.set`




