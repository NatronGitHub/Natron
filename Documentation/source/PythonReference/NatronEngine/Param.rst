.. module:: NatronEngine
.. _Param:

Param
*****

**Inherited by:** :ref:`ParametricParam`, :ref:`PageParam`, :ref:`GroupParam`, :ref:`ButtonParam`, :ref:`AnimatedParam`, :ref:`StringParamBase`, :ref:`PathParam`, :ref:`OutputFileParam`, :ref:`FileParam`, :ref:`StringParam`, :ref:`BooleanParam`, :ref:`ChoiceParam`, :ref:`ColorParam`, :ref:`DoubleParam`, :ref:`Double2DParam`, :ref:`Double3DParam`, :ref:`IntParam`, :ref:`Int2DParam`, :ref:`Int3DParam`

Synopsis
--------

This is the base class for all parameters. Parameters are the controls found in the settings
panel of a node. See :ref:`details here<details>`.

Functions
^^^^^^^^^

*    def :meth:`getAddNewLine<NatronEngine.Param.getAddNewLine>` ()
*    def :meth:`getCanAnimate<NatronEngine.Param.getCanAnimate>` ()
*    def :meth:`getEvaluateOnChange<NatronEngine.Param.getEvaluateOnChange>` ()
*    def :meth:`getHelp<NatronEngine.Param.getHelp>` ()
*    def :meth:`getIsAnimationEnabled<NatronEngine.Param.getIsAnimationEnabled>` ()
*    def :meth:`getIsEnabled<NatronEngine.Param.getIsEnabled>` ([dimension=0])
*    def :meth:`getIsPersistant<NatronEngine.Param.getIsPersistant>` ()
*    def :meth:`getIsVisible<NatronEngine.Param.getIsVisible>` ()
*    def :meth:`getLabel<NatronEngine.Param.getLabel>` ()
*    def :meth:`getNumDimensions<NatronEngine.Param.getNumDimensions>` ()
*    def :meth:`getParent<NatronEngine.Param.getParent>` ()
*    def :meth:`getScriptName<NatronEngine.Param.getScriptName>` ()
*    def :meth:`getTypeName<NatronEngine.Param.getTypeName>` ()
*    def :meth:`setAddNewLine<NatronEngine.Param.setAddNewLine>` (a)
*    def :meth:`setAnimationEnabled<NatronEngine.Param.setAnimationEnabled>` (e)
*    def :meth:`setEnabled<NatronEngine.Param.setEnabled>` (enabled[, dimension=0])
*    def :meth:`setEvaluateOnChange<NatronEngine.Param.setEvaluateOnChange>` (eval)
*    def :meth:`setHelp<NatronEngine.Param.setHelp>` (help)
*    def :meth:`setPersistant<NatronEngine.Param.setPersistant>` (persistant)
*    def :meth:`setVisible<NatronEngine.Param.setVisible>` (visible)

.. _details:

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

	* addNewLine:	When True, the next parameter declared will be on the same line as this parameter
	
	* canAnimate: This is a static property that you cannot control which tells whether animation can be enabled for a specific type of parameter
	
	* animationEnabled: For all parameters that have canAnimate=True, this property controls whether this parameter should be able to animate (= have keyframes) or not
	
	* evaluateOnChange: This property controls whether a new render should be issues when the value of this parameter changes
	
	* help: This is the tooltip visible when hovering the parameter with the mouse
	
	* enabled: Should this parameter be editable by the user or not. Generally, disabled parameters have their text in painted in black.
	
	* visible: Should this parameter be visible in the user interface or not
	
	* persistant: If true then the parameter value will be saved in the project
	
	* dimension: How many dimensions this parameter has. For instance a :doc:`Double3DParam` has 3 dimensions. A :doc:`ParametricParam` has as many dimensions as there are curves.

Note that most of the functions in the API of Params take a *dimension* parameter. This is a 0-based index of the dimension on which to operate.
				 	

The following table sums up the different properties for all parameters including type-specific properties not listed above.


Note that  most of the properties are not dynamic:
they need to be set before calling :func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>` which will create the GUI for these parameters.

.. warning::

	A non-dynamic property can no longer be changed once refreshUserParamsGUI() has been called.
	
For non *user-parameters* (i.e: parameters that were defined by the underlying OpenFX plug-in), only 
their **dynamic** properties can be changed since  :func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>`
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
| persistent        | bool         |   yes        |         setPersistant(*)       | getIsPersistant      | True                  |
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

.. method:: NatronEngine.Param.getAddNewLine()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the *next* parameter defined after this one should be on the same line or not.




.. method:: NatronEngine.Param.getCanAnimate()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether this class can have any animation or not. This cannot be changed.
calling :func:`setAnimationEnabled(True)<NatronEngine.Param.setAnimationEnabled>` will
not enable animation for parameters that cannot animate.




.. method:: NatronEngine.Param.getEvaluateOnChange()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether this parameter can evaluate on change. A parameter evaluating on change
means that a new render will be triggered when its value changes due to a call of one of
the setValue functions.




.. method:: NatronEngine.Param.getHelp()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the help tooltip visible when hovering the parameter with the mouse on the GUI;




.. method:: NatronEngine.Param.getIsAnimationEnabled()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether animation is enabled for this parameter. This is dynamic and can be
changed by :func:`setAnimationEnabled(bool)<NatronEngine.Param.setAnimationEnabled>` if the
parameter *can animate*.
	



.. method:: NatronEngine.Param.getIsEnabled([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the given *dimension* is enabled or not.




.. method:: NatronEngine.Param.getIsPersistant()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether this parameter should be persistant in the project or not.
Non-persistant parameter will not have their value saved when saving a project.




.. method:: NatronEngine.Param.getIsVisible()


    :rtype: :class:`bool<PySide.QtCore.bool>`

	Returns whether the parameter is visible on the user interface or not.




.. method:: NatronEngine.Param.getLabel()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the *label* of the parameter. This is what is displayed in the settings panel
of the node. See :ref:`this section<autoVar>` for an explanation of the difference between
the *label* and the *script name*




.. method:: NatronEngine.Param.getNumDimensions()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of dimensions. For exampel a :doc:`Double3DParam` has 3 dimensions.
A :doc:`ParametricParam` has as many dimensions as there are curves.




.. method:: NatronEngine.Param.getParent()


    :rtype: :class:`NatronEngine.Param`

If this param is within a :doc:`group<GroupParam>`, then the parent will be the group.
Otherwise the param's parent will be the:doc:`page<PageParam>` onto which the param
appears in the settings panel.




.. method:: NatronEngine.Param.getScriptName()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the *script-name* of the param as used internally. The script-name is visible
in the tooltip of the parameter when hovering the mouse over it on the GUI.
See :ref:`this section<autoVar>` for an explanation of the difference between
the *label* and the *script name*




.. method:: NatronEngine.Param.getTypeName()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the type-name of the parameter.




.. method:: NatronEngine.Param.setAddNewLine(a)


    :param a: :class:`bool<PySide.QtCore.bool>`

Set whether the parameter should trigger a new line after its declaration or not.
See :func:`getAddNewLine()<NatronEngine.Param.getAddNewLine>`




.. method:: NatronEngine.Param.setAnimationEnabled(e)


    :param e: :class:`bool<PySide.QtCore.bool>`

Set whether animation should be enabled (= can have keyframes). 
See :func:`getIsAnimationEnabled()<NatronEngine.Param.getIsAnimationEnabled>`




.. method:: NatronEngine.Param.setEnabled(enabled[, dimension=0])


    :param enabled: :class:`bool<PySide.QtCore.bool>`
    :param dimension: :class:`int<PySide.QtCore.int>`

Set whether the given *dimension* of the parameter should be enabled or not.
When disabled, the parameter will be displayed in black and the user will not be able
to edit it.
See :func:`getIsEnabled(dimension)<NatronEngine.Param.getIsEnabled>`




.. method:: NatronEngine.Param.setEvaluateOnChange(eval)


    :param eval: :class:`bool<PySide.QtCore.bool>`

Set whether evaluation should be enabled for this parameter. When True, calling any
function that change the value of the parameter will trigger a new render.
See :func:`getEvaluateOnChange()<NatronEngine.Param.getEvaluateOnChange>`



.. method:: NatronEngine.Param.setHelp(help)


    :param help: :class:`str<NatronEngine.std::string>`

Set the help tooltip of the parameter.
See :func:`getHelp()<NatronEngine.Param.getHelp>`


.. method:: NatronEngine.Param.setPersistant(persistant)


    :param persistant: :class:`bool<PySide.QtCore.bool>`

Set whether this parameter should be persistant or not.
Non persistant parameter will not be saved in the project.
See :func:`getIsPersistant<NatronEngine.Param.getIsPersistant>`




.. method:: NatronEngine.Param.setVisible(visible)


    :param visible: :class:`bool<PySide.QtCore.bool>`
	
Set whether this parameter should be visible or not to the user.
See :func:`getIsVisible()<NatronEngine.Param.getIsVisible>`





