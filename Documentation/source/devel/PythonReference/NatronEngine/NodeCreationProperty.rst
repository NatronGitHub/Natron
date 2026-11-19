.. currentmodule:: NatronEngine
.. _NodeCreationProperty:

NodeCreationProperty
********************

**Inherited by:** IntNodeCreationProperty, BoolNodeCreationProperty,
FloatNodeCreationProperty, StringNodeCreationProperty

Synopsis
--------

Node creation properties are the values of the optional *properties* dictionary
passed to :func:`App.createNode()<NatronEngine.App.createNode>`,
:func:`App.createReader()<NatronEngine.App.createReader>` and
:func:`App.createWriter()<NatronEngine.App.createWriter>`. Each property holds one or
more values of a single type. The keys of the dictionary, and the type and number of
values each key expects, are listed in the description of
:func:`App.createNode()<NatronEngine.App.createNode>`.

For example::

    props = {
        "CreateNodeArgsPropSettingsOpened": NatronEngine.BoolNodeCreationProperty(True),
        "CreateNodeArgsPropNodeInitialPosition": NatronEngine.FloatNodeCreationProperty([100., 200.]),
    }
    blur = app.createNode("net.sf.cimg.CImgBlur", -1, app, props)

NodeCreationProperty is the base class of the four types below. Don't create it
directly: a plain ``NodeCreationProperty()`` holds no value, and createNode silently ignores it.

The four property types have the same functions. Only the type of the values differs.

.. class:: IntNodeCreationProperty([values])

    :param values: :class:`int` or a sequence of :class:`int`

    A property holding integer values.

.. class:: BoolNodeCreationProperty([values])

    :param values: :class:`bool` or a sequence of :class:`bool`

    A property holding boolean values.

.. class:: FloatNodeCreationProperty([values])

    :param values: :class:`float` or a sequence of :class:`float`

    A property holding floating-point values.

.. class:: StringNodeCreationProperty([values])

    :param values: :class:`str` or a sequence of :class:`str`

    A property holding string values.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The four classes have the same two functions; only the type of the values differs.

.. method:: IntNodeCreationProperty.setValue(value[, index=0])

    :param value: :class:`int`
    :param index: :class:`int`

    Sets the value at position *index*. The list of values grows if *index* is past its end.
    A negative *index* is ignored.

.. method:: IntNodeCreationProperty.getValues()

    :rtype: a sequence of :class:`int`

    Returns all the values of the property.

.. method:: BoolNodeCreationProperty.setValue(value[, index=0])

    :param value: :class:`bool`
    :param index: :class:`int`

    Sets the value at position *index*. The list of values grows if *index* is past its end.
    A negative *index* is ignored.

.. method:: BoolNodeCreationProperty.getValues()

    :rtype: a sequence of :class:`bool`

    Returns all the values of the property.

.. method:: FloatNodeCreationProperty.setValue(value[, index=0])

    :param value: :class:`float`
    :param index: :class:`int`

    Sets the value at position *index*. The list of values grows if *index* is past its end.
    A negative *index* is ignored.

.. method:: FloatNodeCreationProperty.getValues()

    :rtype: a sequence of :class:`float`

    Returns all the values of the property.

.. method:: StringNodeCreationProperty.setValue(value[, index=0])

    :param value: :class:`str`
    :param index: :class:`int`

    Sets the value at position *index*. The list of values grows if *index* is past its end.
    A negative *index* is ignored.

.. method:: StringNodeCreationProperty.getValues()

    :rtype: a sequence of :class:`str`

    Returns all the values of the property.
