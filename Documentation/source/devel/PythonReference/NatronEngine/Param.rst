.. currentmodule:: NatronEngine
.. _Param:

Param
*****

**Inherited by:** :doc:`ParametricParam`, :doc:`PageParam`, :doc:`GroupParam`, :doc:`ButtonParam`, :doc:`SeparatorParam`, :doc:`AnimatedParam`, :doc:`StringParamBase`, :doc:`PathParam`, :doc:`OutputFileParam`, :doc:`FileParam`, :doc:`StringParam`, :doc:`BooleanParam`, :doc:`ChoiceParam`, :doc:`ColorParam`, :doc:`DoubleParam`, :doc:`Double2DParam`, :doc:`Double3DParam`, :doc:`IntParam`, :doc:`Int2DParam`, :doc:`Int3DParam`

Synopsis
--------

This is the base class for all parameters. Parameters are the controls found in the settings
panel of a node. See :ref:`details here<paramdetails>`.

Functions
^^^^^^^^^

- def :meth:`copy<NatronEngine.Param.copy>` (param[, dimension=-1])
- def :meth:`curve<NatronEngine.Param.curve>` (time[, dimension=-1])
- def :meth:`getAddNewLine<NatronEngine.Param.getAddNewLine>` ()
- def :meth:`getCanAnimate<NatronEngine.Param.getCanAnimate>` ()
- def :meth:`getEvaluateOnChange<NatronEngine.Param.getEvaluateOnChange>` ()
- def :meth:`getHelp<NatronEngine.Param.getHelp>` ()
- def :meth:`getIsAnimationEnabled<NatronEngine.Param.getIsAnimationEnabled>` ()
- def :meth:`getIsEnabled<NatronEngine.Param.getIsEnabled>` ([dimension=0])
- def :meth:`getIsPersistent<NatronEngine.Param.getIsPersistent>` ()
- def :meth:`getIsVisible<NatronEngine.Param.getIsVisible>` ()
- def :meth:`getLabel<NatronEngine.Param.getLabel>` ()
- def :meth:`getNumDimensions<NatronEngine.Param.getNumDimensions>` ()
- def :meth:`getParent<NatronEngine.Param.getParent>` ()
- def :meth:`getScriptName<NatronEngine.Param.getScriptName>` ()
- def :meth:`getTypeName<NatronEngine.Param.getTypeName>` ()
- def :meth:`random<NatronEngine.Param.random` ([min=0.,max=1.])
- def :meth:`random<NatronEngine.Param.random` (seed)
- def :meth:`randomInt<NatronEngine.Param.randomInt` (min,max)
- def :meth:`randomInt<NatronEngine.Param.randomInt` (seed)
- def :meth:`setAddNewLine<NatronEngine.Param.setAddNewLine>` (a)
- def :meth:`setAnimationEnabled<NatronEngine.Param.setAnimationEnabled>` (e)
- def :meth:`setEnabled<NatronEngine.Param.setEnabled>` (enabled[, dimension=0])
- def :meth:`setEnabledByDefault<NatronEngine.Param.setEnabledByDefault>` (enabled)
- def :meth:`setEvaluateOnChange<NatronEngine.Param.setEvaluateOnChange>` (eval)
- def :meth:`setIconFilePath<NatronEngine.Param.setIconFilePath>` (icon)
- def :meth:`setHelp<NatronEngine.Param.setHelp>` (help)
- def :meth:`setPersistent<NatronEngine.Param.setPersistent>` (persistent)
- def :meth:`setVisible<NatronEngine.Param.setVisible>` (visible)
- def :meth:`setVisibleByDefault<NatronEngine.Param.setVisibleByDefault>` (visible)
- def :meth:`setAsAlias<NatronEngine.Param.setAsAlias>` (otherParam)
- def :meth:`slaveTo<NatronEngine.Param.slaveTo>` (otherParam, thisDimension, otherDimension)
- def :meth:`unslave<NatronEngine.Param.unslave>` (dimension)

.. _paramdetails:

Detailed Description
--------------------

The Param object can be used to control a specific parameter of a node.
There are different types of parameters, ranging from the single
checkbox (boolean) to parametric curves.
Each type of parameter has specific functions to control the parameter according to
its internal value type.
In this base class, all common functionalities for parameters have been gathered.

.. warning::
    Note that since each child class has a different value type, all the functions to set/get values, and set/get keyframes
    are specific for each class.

A Param can have several functions to control some properties, namely:

    * addNewLine:   When True, the next parameter declared will be on the same line as this parameter

    * canAnimate: This is a static property that you cannot control which tells whether animation can be enabled for a specific type of parameter

    * animationEnabled: For all parameters that have canAnimate=True, this property controls whether this parameter should be able to animate (= have keyframes) or not

    * evaluateOnChange: This property controls whether a new render should be issues when the value of this parameter changes

    * help: This is the tooltip visible when hovering the parameter with the mouse

    * enabled: Should this parameter be editable by the user or not. Generally, disabled parameters have their text in painted in black.

    * visible: Should this parameter be visible in the user interface or not

    * persistent: If true then the parameter value will be saved in the project

    * dimension: How many dimensions this parameter has. For instance a :doc:`Double3DParam` has 3 dimensions. A :doc:`ParametricParam` has as many dimensions as there are curves.

Note that most of the functions in the API of Params take a *dimension* parameter. This is a 0-based index of the dimension on which to operate.


The following table sums up the different properties for all parameters including type-specific properties not listed above.


Note that  most of the properties are not dynamic:
they need to be set before calling :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>` which will create the GUI for these parameters.

.. warning::

    A non-dynamic property can no longer be changed once refreshUserParamsGUI() has been called.

For non *user-parameters* (i.e: parameters that were defined by the underlying OpenFX plug-in), only
their **dynamic** properties can be changed since  :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
will only refresh user parameters.

If a Setter function contains a (*) that means it can only be called for user parameters,
it has no effect on already declared non-user parameters.

+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| Name:             | Type:        |   Dynamic:   |         Setter:                | Getter:              | Default:              |
+===================+==============+==============+================================+======================+=======================+
| name              | string       |   no         |         None                   | getScriptName        | ""                    |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| label             | string       |   no         |         None                   | getLabel             | ""                    |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| help              | string       |   yes        |         setHelp(*)             | getHelp              | ""                    |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| addNewLine        | bool         |   no         |         setAddNewLine(*)       | getAddNewLine        | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| persistent        | bool         |   yes        |         setPersistent(*)       | getIsPersistent      | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| evaluatesOnChange | bool         |   yes        |         setEvaluateOnChange(*) | getEvaluateOnChange  | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| animates          | bool         |   no         |         setAnimationEnabled(*) | getIsAnimationEnabled| See :ref:`(1)<(1)>`   |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| visible           | bool         |   yes        |         setVisible             | getIsVisible         | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| enabled           | bool         |   yes        |         setEnabled             | getIsEnabled         | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
|                                                                                                                                 |
| *Properties on IntParam, Int2DParam, Int3DParam, DoubleParam, Double2DParam, Double3DParam, ColorParam only:*                   |
|                                                                                                                                 |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| min               | int/double   |   yes        |         setMinimum(*)          |  getMinimum          |  INT_MIN              |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| max               | int/double   |   yes        |         setMaximum(*)          |  getMaximum          |  INT_MAX              |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| displayMin        | int/double   |   yes        |         setDisplayMinimum(*)   |  getDisplayMinimum   |  INT_MIN              |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| displayMax        | int/double   |   yes        |         setDisplayMaximum(*)   |  getDisplayMaximum   |  INT_MAX              |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
|                                                                                                                                 |
| *Properties on ChoiceParam only:*                                                                                               |
|                                                                                                                                 |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| options           | list<string> |   yes        |         setOptions/addOption(*)|  getOption           |  empty list           |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
|                                                                                                                                 |
| *Properties on FileParam, OutputFileParam only:*                                                                                |
|                                                                                                                                 |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| sequenceDialog    | bool         |   yes        |         setSequenceEnabled(*)  |  None                |  False                |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
|                                                                                                                                 |
| *Properties on StringParam only:*                                                                                               |
|                                                                                                                                 |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| type              | TypeEnum     |   no         |         setType(*)             |  None                |  eStringTypeDefault   |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
|                                                                                                                                 |
| *Properties on PathParam only:*                                                                                                 |
|                                                                                                                                 |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| multiPathTable    | bool         |   no         |         setAsMultiPathTable(*) |  None                |  False                |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
|                                                                                                                                 |
| *Properties on GroupParam only:*                                                                                                |
|                                                                                                                                 |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| isTab             | bool         |   no         |         setAsTab(*)            |  None                |   False               |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+

   .. _(1):

    (1): animates is set to True by default only if it is one of the following parameters:
    IntParam Int2DParam Int3DParam
    DoubleParam Double2DParam Double3DParam
    ColorParam

    Note that ParametricParam , GroupParam, PageParam, ButtonParam, FileParam, OutputFileParam,
    PathParam cannot animate at all.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: Param.copy(other [, dimension=-1])

    :param other: :class:`Param`
    :param dimension: :class:`int`
    :rtype: :class:`bool`

Copies the *other* parameter values, animation and expressions at the given *dimension*.
If *dimension* is -1, all dimensions in **min(getNumDimensions(), other.getNumDimensions())** will
be copied.

.. note::
    Note that types must be convertible:

    IntParam,DoubleParam, ChoiceParam, ColorParam and BooleanParam can convert between types but StringParam cannot.

.. warning::

    When copying a parameter, only values are copied, not properties, hence if copying a
    choice parameter, make sure that the value you copy has a meaning to the receiver otherwise
    you might end-up with an undefined behaviour, e.g.:

    If ChoiceParam1 has 3 entries and the current index is 2 and ChoiceParam2 has 15 entries
    and current index is 10, copying ChoiceParam2 to ChoiceParam1 will end-up in undefined behaviour.


This function returns **True** upon success and **False** otherwise.


.. method:: Param.curve(time [, dimension=-1])

    :param time: :class:`float`
    :param dimension: :class:`int`
    :rtype: :class:`float`

    If this parameter has an animation curve on the given *dimension*, then the value of
    that curve at the given *time* is returned. If the parameter has an expression on top
    of the animation curve, the expression will be ignored, ie.g. the value of the animation
    curve will still be returned.
    This is useful to write custom expressions for motion design such as looping, reversing, etc...

    .. note::

        On a string parameter (:doc:`StringParamBase` and its subclasses), this returns the
        0-based index of the keyframe at or before *time* (0 before the first keyframe).
        The value is constant between keyframes: it is never interpolated.
        To get the string itself, use
        :func:`getValueAtTime()<NatronEngine.StringParamBase.getValueAtTime>`.


.. method:: Param.getAddNewLine()


    :rtype: :class:`bool`

Returns whether the parameter is on a new line or not.




.. method:: Param.getCanAnimate()


    :rtype: :class:`bool`

Returns whether this class can have any animation or not. This cannot be changed.
calling :func:`setAnimationEnabled(True)<NatronEngine.Param.setAnimationEnabled>` will
not enable animation for parameters that cannot animate.




.. method:: Param.getEvaluateOnChange()


    :rtype: :class:`bool`

Returns whether this parameter can evaluate on change. A parameter evaluating on change
means that a new render will be triggered when its value changes due to a call of one of
the setValue functions.




.. method:: Param.getHelp()


    :rtype: :class:`str`

Returns the help tooltip visible when hovering the parameter with the mouse on the GUI;




.. method:: Param.getIsAnimationEnabled()


    :rtype: :class:`bool`

Returns whether animation is enabled for this parameter. This is dynamic and can be
changed by :func:`setAnimationEnabled(bool)<NatronEngine.Param.setAnimationEnabled>` if the
parameter *can animate*.




.. method:: Param.getIsEnabled([dimension=0])


    :param dimension: :class:`int`
    :rtype: :class:`bool`

Returns whether the given *dimension* is enabled or not.




.. method:: Param.getIsPersistent()


    :rtype: :class:`bool`

Returns whether this parameter should be persistent in the project or not.
Non-persistent parameter will not have their value saved when saving a project.




.. method:: Param.getIsVisible()


    :rtype: :class:`bool`

    Returns whether the parameter is visible on the user interface or not.




.. method:: Param.getLabel()


    :rtype: :class:`str`

Returns the *label* of the parameter. This is what is displayed in the settings panel
of the node. See :ref:`this section<autoVar>` for an explanation of the difference between
the *label* and the *script name*




.. method:: Param.getNumDimensions()


    :rtype: :class:`int`

Returns the number of dimensions. For exampel a :doc:`Double3DParam` has 3 dimensions.
A :doc:`ParametricParam` has as many dimensions as there are curves.




.. method:: Param.getParent()


    :rtype: :class:`NatronEngine.Param`

If this param is within a :doc:`group<GroupParam>`, then the parent will be the group.
Otherwise the param's parent will be the:doc:`page<PageParam>` onto which the param
appears in the settings panel.




.. method:: Param.getScriptName()


    :rtype: :class:`str`

Returns the *script-name* of the param as used internally. The script-name is visible
in the tooltip of the parameter when hovering the mouse over it on the GUI.
See :ref:`this section<autoVar>` for an explanation of the difference between
the *label* and the *script name*




.. method:: Param.getTypeName()


    :rtype: :class:`str`

Returns the type-name of the parameter.


.. method:: Param.random([min=0., max=1.])

    :param min: :class:`float`
    :param max: :class:`float`
    :rtype: :class:`float`

Returns a pseudo-random value in the interval \[*min*, *max*\[: the next value of a sequence
that belongs to the parameter. Each evaluation of the expression of the parameter starts that
sequence again from the frame, the dimension and the state of the node, so in the expression of
the parameter itself the value is the same each time a frame is computed, and can be reproduced
exactly. Elsewhere (in a script, or when called on another parameter in an expression), the value
depends on the calls made before: use :func:`random(min,max,time,seed)<NatronEngine.Param.random>`.


.. note::

    Note that if you are calling multiple times random() in the same parameter expression,
    each call would return a different value, but they would all return the same value again
    if the expressions is interpreted at the same time, e.g.:

        # Would always return the same value at a given timeline's time.
        random() - random()

Note that you can ensure that random() returns a given value by calling the overloaded
function :func:`random(min,max,time,seed)<NatronEngine.Param.random>` instead.

.. method:: Param.random(min, max, time, [seed=0])
    :noindex:

    :param min: :class:`float`
    :param max: :class:`float`
    :param time: :class:`float`
    :param seed: :class:`int`
    :rtype: :class:`float`

Same as :func:`random()<NatronEngine.Param.random>` but takes **time** and **seed** in parameters to control
the value returned by the function. E.g::

    ret = random(0,1,frame,2) - random(0,1,frame,2)
    # ret == 0 always

.. method:: Param.randomInt(min,max)

    :param min: :class:`int`
    :param max: :class:`int`
    :rtype: :class:`int`

Same as  :func:`random(min,max)<NatronEngine.Param.random>` but returns an integer in the
range [*min*,*max*[

.. method:: Param.randomInt(min, max, time, [seed=0])
    :noindex:

    :param min: :class:`int`
    :param max: :class:`int`
    :param time: :class:`float`
    :param seed: :class:`int`
    :rtype: :class:`int`

Same as :func:`random(min,max,time,seed)<NatronEngine.Param.random>` but returns an integer in the range
[0, INT_MAX] instead.


.. method:: Param.setAddNewLine(a)


    :param a: :class:`bool`

Set whether the parameter should be on a new line or not.
See :func:`getAddNewLine()<NatronEngine.Param.getAddNewLine>`




.. method:: Param.setAnimationEnabled(e)


    :param e: :class:`bool`

Set whether animation should be enabled (= can have keyframes).
See :func:`getIsAnimationEnabled()<NatronEngine.Param.getIsAnimationEnabled>`




.. method:: Param.setEnabled(enabled[, dimension=0])


    :param enabled: :class:`bool`
    :param dimension: :class:`int`

Set whether the given *dimension* of the parameter should be enabled or not.
When disabled, the parameter will be displayed in black and the user will not be able
to edit it.
See :func:`getIsEnabled(dimension)<NatronEngine.Param.getIsEnabled>`

.. method:: Param.setEnabledByDefault(enabled)


    :param enabled: :class:`bool`

Set whether the parameter should be enabled or not by default.
When disabled, the parameter will be displayed in black and the user will not be able
to edit it.


.. method:: Param.setEvaluateOnChange(eval)


    :param eval: :class:`bool`

Set whether evaluation should be enabled for this parameter. When True, calling any
function that change the value of the parameter will trigger a new render.
See :func:`getEvaluateOnChange()<NatronEngine.Param.getEvaluateOnChange>`


.. method:: Param.setIconFilePath(icon)


    :param icon: :class:`str`

Icon file path for the label. This should be either an absolute path or
a file-path relative to a path in the NATRON_PLUGIN_PATH. The icon will replace the
label of the parameter.





.. method:: Param.setHelp(help)


    :param help: :class:`str`

Tooltip help string for the parameter.
See :func:`getHelp()<NatronEngine.Param.getHelp>`


.. method:: Param.setPersistent(persistent)


    :param persistent: :class:`bool`

Set whether this parameter should be persistent or not.
Non persistent parameter will not be saved in the project.
See :func:`getIsPersistent<NatronEngine.Param.getIsPersistent>`




.. method:: Param.setVisible(visible)


    :param visible: :class:`bool`

Set whether this parameter should be visible or not to the user.
See :func:`getIsVisible()<NatronEngine.Param.getIsVisible>`

.. method:: Param.setVisibleByDefault(visible)


    :param visible: :class:`bool`

Set whether this parameter should be visible or not to the user in its default state.


.. method:: Param.setAsAlias(otherParam)

    :param otherParam: :class:`Param<NatronEngine.Param>`
    :rtype: :class:`bool`


Set this parameter as an alias of *otherParam*.
They need to be both of the same *type* and of the same *dimension*.
This parameter will control *otherParam* entirely and in case of a choice param, its
drop-down menu will be updated whenever the *otherParam* menu is updated.

This is used generally to make user parameters on groups with the "Pick" option of the
"Manage User Parameters" dialog.


.. method:: Param.slaveTo(otherParam, thisDimension, otherDimension)

    :param otherParam: :class:`Param<NatronEngine.Param>`
    :param thisDimension: :class:`int`
    :param otherDimension: :class:`int`
    :rtype: :class:`bool`

Set this parameter as a slave of *otherParam*.
They need to be both of the same *type* but may vary in dimension, as long as
*thisDimension* is valid according to the number of dimensions of this parameter and
*otherDimension* is valid according to the number of dimensions of *otherParam*.

This parameter *thisDimension* will be controlled entirely by the *otherDimension* of
*otherParam* until a call to :func:`unslave(thisDimension)<NatronEngine.Param.unslave>` is made

.. method:: Param.unslave(dimension)

    :param dimension: :class:`int`


If the given *dimension* of this parameter was previously slaved, then this function will
remove the link between parameters, and the user will be free again to use this parameter
as any other.

.. note::

     The animation and values that were present before the link will remain.
