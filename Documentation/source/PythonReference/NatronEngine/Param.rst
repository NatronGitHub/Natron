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
they need to be set before calling refreshUserParamsGUI() which will create the GUI for these parameters.

.. warning::

	A non-dynamic property can no longer be changed once refreshUserParamsGUI() has been called.
	
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


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getCanAnimate()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getEvaluateOnChange()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getHelp()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.getIsAnimationEnabled()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getIsEnabled([dimension=0])


    :param dimension: :class:`PySide.QtCore.int`
    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getIsPersistant()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getIsVisible()


    :rtype: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.getLabel()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.getNumDimensions()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Param.getParent()


    :rtype: :class:`NatronEngine.Param`






.. method:: NatronEngine.Param.getScriptName()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.getTypeName()


    :rtype: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.setAddNewLine(a)


    :param a: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setAnimationEnabled(e)


    :param e: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setEnabled(enabled[, dimension=0])


    :param enabled: :class:`PySide.QtCore.bool`
    :param dimension: :class:`PySide.QtCore.int`






.. method:: NatronEngine.Param.setEvaluateOnChange(eval)


    :param eval: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setHelp(help)


    :param help: :class:`NatronEngine.std::string`






.. method:: NatronEngine.Param.setPersistant(persistant)


    :param persistant: :class:`PySide.QtCore.bool`






.. method:: NatronEngine.Param.setVisible(visible)


    :param visible: :class:`PySide.QtCore.bool`







