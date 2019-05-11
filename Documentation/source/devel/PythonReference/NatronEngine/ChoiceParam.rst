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

- def :meth:`addOption<NatronEngine.ChoiceParam.addOption>` (optionID, optionLabel, optionHelp)
- def :meth:`get<NatronEngine.ChoiceParam.get>` ([view="Main"])
- def :meth:`get<NatronEngine.ChoiceParam.get>` (frame[,view="Main"])
- def :meth:`getActiveOption<NatronEngine.ChoiceParam.getActiveOption>` ([view="Main"])
- def :meth:`getDefaultValue<NatronEngine.ChoiceParam.getDefaultValue>` ()
- def :meth:`getOption<NatronEngine.ChoiceParam.getOption>` (index)
- def :meth:`getNumOptions<NatronEngine.ChoiceParam.getNumOptions>` ()
- def :meth:`getOptions<NatronEngine.ChoiceParam.getOptions>` ()
- def :meth:`getValue<NatronEngine.ChoiceParam.getValue>` ([view="Main"])
- def :meth:`getValueAtTime<NatronEngine.ChoiceParam.getValueAtTime>` (time[,view="Main"])
- def :meth:`restoreDefaultValue<NatronEngine.ChoiceParam.restoreDefaultValue>` ([view="All"])
- def :meth:`set<NatronEngine.ChoiceParam.set>` (x[,view="All"])
- def :meth:`set<NatronEngine.ChoiceParam.set>` (x, frame[,view="All"])
- def :meth:`set<NatronEngine.ChoiceParam.set>` (label[,view="All"])
- def :meth:`setDefaultValue<NatronEngine.ChoiceParam.setDefaultValue>` (value)
- def :meth:`setDefaultValue<NatronEngine.ChoiceParam.setDefaultValue>` (label)
- def :meth:`setOptions<NatronEngine.ChoiceParam.setOptions>` (options)
- def :meth:`setValue<NatronEngine.ChoiceParam.setValue>` (value[,view="All"])
- def :meth:`setValueAtTime<NatronEngine.ChoiceParam.setValueAtTime>` (value, time[,view="All"])

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


.. method:: NatronEngine.ChoiceParam.addOption(optionID, optionLabel, optionHelp)


    :param optionID: :class:`str<PySide.QtCore.QString>`
    :param optionLabel: :class:`str<PySide.QtCore.QString>`
    :param optionHelp: :class:`str<PySide.QtCore.QString>`

Adds a new option to the menu.
The *optionID* is a unique identifier for the option and is not displayed to the user.
The *optionLabel* is the label visible in the drop-down menu for the user.
If empty, the *optionLabel* will be automatically set to be the same as the *optionID*
If *optionHelp* is not empty, it will be displayed when the user
hovers the entry with the mouse.



.. method:: NatronEngine.ChoiceParam.get(frame[, view="Main"])


    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`int<PySide.QtCore.int>`

Get the value of the parameter at the given *frame* and *view*.




.. method:: NatronEngine.ChoiceParam.get([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`int<PySide.QtCore.int>`

Get the value of the parameter at the current timeline's time for the given *view*.




.. method:: NatronEngine.ChoiceParam.getDefaultValue()


    :rtype: :class:`int<PySide.QtCore.int>`

Get the default value for this parameter.


.. method:: NatronEngine.ChoiceParam.getActiveOption([view="Main"])


    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`PyTuple`

Get the active menu entry for the given *view*.
Note that the active entry may not be present in the options returned by the
getOptions() function if the menu was changed. In this case the option will
be displayed in italic in the user interface.
The tuple is composed of 3 strings: the optionID, optionLabel and optionHint.
The optionID is what uniquely identifies the entry in the drop-down menu.
The optionLabel is what is visible in the user interface.
The optionHint is the help for the entry visible in the tooltip.




.. method:: NatronEngine.ChoiceParam.getOption(index)


    :param index: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`PyTuple`

Get the menu entry at the given *index*.
The tuple is composed of 3 strings: the optionID, optionLabel and optionHint.
The optionID is what uniquely identifies the entry in the drop-down menu.
The optionLabel is what is visible in the user interface.
The optionHint is the help for the entry visible in the tooltip.



.. method:: NatronEngine.ChoiceParam.getNumOptions()

    :rtype: :class:`int`

Returns the number of menu entries.

.. method:: NatronEngine.ChoiceParam.getOptions()

    :rtype: :class:`sequence`

Returns a sequence of tuple with all menu entries from top to bottom.
Each tuple is composed of 3 strings: the optionID, optionLabel and optionHint.
The optionID is what uniquely identifies the entry in the drop-down menu.
The optionLabel is what is visible in the user interface.
The optionHint is the help for the entry visible in the tooltip.

.. method:: NatronEngine.ChoiceParam.getValue()


    :rtype: :class:`int<PySide.QtCore.int>`

Same as :func:`get()<NatronEngine.ChoiceParam.get>`




.. method:: NatronEngine.ChoiceParam.getValueAtTime(time[,view="Main"])


    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`float<PySide.QtCore.float>`

Same as :func:`get(frame,view)<NatronEngine.ChoiceParam.get>`




.. method:: NatronEngine.ChoiceParam.restoreDefaultValue([view="All"])


    :param view: :class:`str<PySide.QtCore.QString>`

Removes all animation and expression set on this parameter for the given *view* and set the value
to be the default value.




.. method:: NatronEngine.ChoiceParam.set(x [, view="All"])


    :param x: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`

Set the value of this parameter to be *x* for the given view.
 If this parameter is animated (see :func:`getIsAnimated(dimension, view)<NatronEngine.AnimatedParam.getIsAnimated>`
then this function will automatically add a keyframe at the timeline's current time.




.. method:: NatronEngine.ChoiceParam.set(x, frame [, view="All"])


    :param x: :class:`int<PySide.QtCore.int>`
    :param frame: :class:`float<PySide.QtCore.float>`
    :param view: :class:`str<PySide.QtCore.QString>`

Set a new keyframe on the parameter with the value *x* at the given *frame* and *view*.



.. method:: NatronEngine.ChoiceParam.set(label[, view="All"])


    :param label: :class:`str<NatronEngine.std::string>`
    :param view: :class:`str<PySide.QtCore.QString>`

Set the value of this parameter given a *label*. The *label* must match an existing option.
Strings will be compared without case sensitivity. If not found, nothing happens.


.. method:: NatronEngine.ChoiceParam.setDefaultValue(value)


    :param value: :class:`int<PySide.QtCore.int>`


Set the default *value* for this parameter.

.. method:: NatronEngine.ChoiceParam.setDefaultValue(ID)


    :param label: :class:`str<PySide.QtCore.QString>`


Set the default value from the *ID* for this parameter. The *ID* must match an existing optionID.
Strings will be compared without case sensitivity. If not found, nothing happens.



.. method:: NatronEngine.ChoiceParam.setOptions(options)


    :param options: class::`sequence`

Clears all existing entries in the menu and add all entries contained in *options*
to the menu.
The options is a list of tuples.
Each tuple is composed of 3 strings: the optionID, optionLabel and optionHint.
The optionID is what uniquely identifies the entry in the drop-down menu.
The optionLabel is what is visible in the user interface.
The optionHint is the help for the entry visible in the tooltip.



.. method:: NatronEngine.ChoiceParam.setValue(value[, view="All"])


    :param value: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`

Same as :func:`set<NatronEngine.ChoiceParam.setValue>`


.. method:: NatronEngine.ChoiceParam.setValueAtTime(value, time[,view="All"])


    :param value: :class:`int<PySide.QtCore.int>`
    :param time: :class:`int<PySide.QtCore.int>`

Same as :func:`set(time)<NatronEngine.ChoiceParam.set`





