.. module:: NatronEngine
.. _ChoiceParam:

ChoiceParam
***********

**Inherits** : :doc:`AnimatedParam`

Synopsis
--------

A choice parameter holds an integer value which corresponds to a choice.
See :ref:`detailed description<choice.details>` below.

Functions
^^^^^^^^^

- def :meth:`addOption<NatronEngine.ChoiceParam.addOption>` (option, help)
- def :meth:`get<NatronEngine.ChoiceParam.get>` ()
- def :meth:`get<NatronEngine.ChoiceParam.get>` (frame)
- def :meth:`getDefaultValue<NatronEngine.ChoiceParam.getDefaultValue>` ()
- def :meth:`getOption<NatronEngine.ChoiceParam.getOption>` (index)
- def :meth:`getNumOptions<NatronEngine.ChoiceParam.getNumOptions>` ()
- def :meth:`getOptions<NatronEngine.ChoiceParam.getOptions>` ()
- def :meth:`getValue<NatronEngine.ChoiceParam.getValue>` ()
- def :meth:`getValueAtTime<NatronEngine.ChoiceParam.getValueAtTime>` (time)
- def :meth:`restoreDefaultValue<NatronEngine.ChoiceParam.restoreDefaultValue>` ()
- def :meth:`set<NatronEngine.ChoiceParam.set>` (x)
- def :meth:`set<NatronEngine.ChoiceParam.set>` (x, frame)
- def :meth:`set<NatronEngine.ChoiceParam.set>` (label)
- def :meth:`setDefaultValue<NatronEngine.ChoiceParam.setDefaultValue>` (value)
- def :meth:`setDefaultValue<NatronEngine.ChoiceParam.setDefaultValue>` (label)
- def :meth:`setOptions<NatronEngine.ChoiceParam.setOptions>` (options)
- def :meth:`setValue<NatronEngine.ChoiceParam.setValue>` (value)
- def :meth:`setValueAtTime<NatronEngine.ChoiceParam.setValueAtTime>` (value, time)

.. _choice.details:

Detailed Description
--------------------

A choice is represented as a drop-down (combobox) in the user interface:

.. figure:: choiceParam.png

You can add options to the menu using the :func:`addOption(option, help)<NatronEngine.ChoiceParam.addOption>` function.
You can also set them all at once using the :func:`setOptions(options)<NatronEngine.ChoiceParam.setOptions>` function.

The value held internally is a 0-based index corresponding to an entry of the menu.
the choice parameter behaves much like an :doc:`IntParam`.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.ChoiceParam.addOption(option, help)


    :param option: :class:`str<NatronEngine.std::string>`
    :param help: :class:`str<NatronEngine.std::string>`

Adds a new *option* to the menu. If *help* is not empty, it will be displayed when the user
hovers the entry with the mouse.



.. method:: NatronEngine.ChoiceParam.get(frame)


    :param frame: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`int<PySide.QtCore.int>`

Get the value of the parameter at the given *frame*.




.. method:: NatronEngine.ChoiceParam.get()


    :rtype: :class:`int<PySide.QtCore.int>`

Get the value of the parameter at the current timeline's time.




.. method:: NatronEngine.ChoiceParam.getDefaultValue()


    :rtype: :class:`int<PySide.QtCore.int>`

Get the default value for this parameter.




.. method:: NatronEngine.ChoiceParam.getOption(index)


    :param index: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`str<NatronEngine.std::string>`

Get the menu entry at the given *index*.



.. method:: NatronEngine.ChoiceParam.getNumOptions()

    :rtype: :class:`int`

Returns the number of menu entries.

.. method:: NatronEngine.ChoiceParam.getOptions()

    :rtype: :class:`sequence`

Returns a sequence of string with all menu entries from top to bottom.

.. method:: NatronEngine.ChoiceParam.getValue()


    :rtype: :class:`int<PySide.QtCore.int>`

Same as :func:`get()<NatronEngine.ChoiceParam.get>`




.. method:: NatronEngine.ChoiceParam.getValueAtTime(time)


    :param time: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

Same as :func:`get(frame)<NatronEngine.ChoiceParam.get>`




.. method:: NatronEngine.ChoiceParam.restoreDefaultValue()



Removes all animation and expression set on this parameter and set the value
to be the default value.




.. method:: NatronEngine.ChoiceParam.set(x)


    :param x: :class:`int<PySide.QtCore.int>`

Set the value of this parameter to be *x*. If this parameter is animated (see :func:`getIsAnimated(dimension)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.




.. method:: NatronEngine.ChoiceParam.set(x, frame)


    :param x: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`

Set a new keyframe on the parameter with the value *x* at the given *frame*.



.. method:: NatronEngine.ChoiceParam.set(label)


    :param label: :class:`str<NatronEngine.std::string>`

Set the value of this parameter given a *label*. The *label* must match an existing option.
Strings will be compared without case sensitivity. If not found, nothing happens.


.. method:: NatronEngine.ChoiceParam.setDefaultValue(value)


    :param value: :class:`int<PySide.QtCore.int>`


Set the default *value* for this parameter.

.. method:: NatronEngine.ChoiceParam.setDefaultValue(label)


    :param label: :class:`str<Natron.std::string>`


Set the default value from the *label* for this parameter. The *label* must match an existing option.
Strings will be compared without case sensitivity. If not found, nothing happens.



.. method:: NatronEngine.ChoiceParam.setOptions(options)


    :param options: class::`sequence`

Clears all existing entries in the menu and add all entries contained in *options*
to the menu.



.. method:: NatronEngine.ChoiceParam.setValue(value)


    :param value: :class:`int<PySide.QtCore.int>`

Same as :func:`set<NatronEngine.ChoiceParam.setValue>`




.. method:: NatronEngine.ChoiceParam.setValueAtTime(value, time)


    :param value: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`

Same as :func:`set(time)<NatronEngine.ChoiceParam.set`





