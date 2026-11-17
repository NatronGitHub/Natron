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

NodeCreationProperty is the base class of the four types below. You don't create
it directly.

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

These functions exist in each of the four classes above.

.. method:: IntNodeCreationProperty.setValue(value[, index=0])

    :param value: a value of the property's type
    :param index: :class:`int`

    Sets the value at position *index*. The list of values grows if *index* is past its end.
    A negative *index* is ignored.

.. method:: IntNodeCreationProperty.getValues()

    :rtype: a sequence of values of the property's type

    Returns all the values of the property.
