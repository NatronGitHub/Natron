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

*    def :meth:`beginChanges<NatronEngine.Param.beginChanges>` ()
*    def :meth:`copy<NatronEngine.Param.copy>` (param[, thisDimension=-1, otherDimension=-1, thisView="All", otherView="All"])
*    def :meth:`curve<NatronEngine.Param.curve>` (time[, dimension=-1, view="Main"])
*    def :meth:`endChanges<NatronEngine.Param.endChanges>` ()
*    def :meth:`getAddNewLine<NatronEngine.Param.getAddNewLine>` ()
*    def :meth:`getCanAnimate<NatronEngine.Param.getCanAnimate>` ()
*    def :meth:`getEvaluateOnChange<NatronEngine.Param.getEvaluateOnChange>` ()
*    def :meth:`getHelp<NatronEngine.Param.getHelp>` ()
*    def :meth:`getIsAnimationEnabled<NatronEngine.Param.getIsAnimationEnabled>` ()
*    def :meth:`getIsEnabled<NatronEngine.Param.getIsEnabled>` ()
*    def :meth:`getIsPersistent<NatronEngine.Param.getIsPersistent>` ()
*    def :meth:`getIsVisible<NatronEngine.Param.getIsVisible>` ()
*    def :meth:`getLabel<NatronEngine.Param.getLabel>` ()
*    def :meth:`getNumDimensions<NatronEngine.Param.getNumDimensions>` ()
*    def :meth:`getParent<NatronEngine.Param.getParent>` ()
*    def :meth:`getParentEffect<NatronEngine.Param.getParentEffect>` ()
*    def :meth:`getParentItemBase<NatronEngine.Param.getParentItemBase>` ()
*    def :meth:`getApp<NatronEngine.Param.getApp>` ()
*    def :meth:`getScriptName<NatronEngine.Param.getScriptName>` ()
*    def :meth:`getTypeName<NatronEngine.Param.getTypeName>` ()
*    def :meth:`getViewerUILayoutType<NatronEngine.Param.getViewerUILayoutType>` ()
*    def :meth:`getViewerUIItemSpacing<NatronEngine.Param.getViewerUIItemSpacing>` ()
*    def :meth:`getViewerUIIconFilePath<NatronEngine.Param.getViewerUIIconFilePath>` ([checked=False])
*    def :meth:`getViewerUILabel<NatronEngine.Param.getViewerUILabel>` ()
*    def :meth:`getHasViewerUI<NatronEngine.Param.getHasViewerUI>` ()
*    def :meth:`getViewerUIVisible<NatronEngine.Param.getViewerUIVisible>` ()
*    def :meth:`isExpressionCacheEnabled<NatronEngine.Param.isExpressionCacheEnabled>` ()
*	 def :meth:`random<NatronEngine.Param.random>` ([min=0.,max=1.])
*	 def :meth:`random<NatronEngine.Param.random>` (seed)
*	 def :meth:`randomInt<NatronEngine.Param.randomInt>` (min,max)
*	 def :meth:`randomInt<NatronEngine.Param.randomInt>` (seed)
*    def :meth:`setAddNewLine<NatronEngine.Param.setAddNewLine>` (a)
*    def :meth:`setAnimationEnabled<NatronEngine.Param.setAnimationEnabled>` (e)
*    def :meth:`setEnabled<NatronEngine.Param.setEnabled>` (enabled)
*    def :meth:`setEvaluateOnChange<NatronEngine.Param.setEvaluateOnChange>` (eval)
*    def :meth:`setIconFilePath<NatronEngine.Param.setIconFilePath>` (icon [,checked=False])
*	 def :meth:`setLabel<NatronEngine.Param.setLabel>` (label)
*    def :meth:`setHelp<NatronEngine.Param.setHelp>` (help)
*    def :meth:`setPersistent<NatronEngine.Param.setPersistent>` (persistent)
*    def :meth:`setExpressionCacheEnabled<NatronEngine.Param.setExpressionCacheEnabled>` (enabled)
*    def :meth:`setVisible<NatronEngine.Param.setVisible>` (visible)
*    def :meth:`setViewerUILayoutType<NatronEngine.Param.setViewerUILayoutType>` (type)
*    def :meth:`setViewerUIItemSpacing<NatronEngine.Param.setViewerUIItemSpacing>` (spacingPx)
*    def :meth:`setViewerUIIconFilePath<NatronEngine.Param.setViewerUIIconFilePath>` (filePath[, checked])
*    def :meth:`setViewerUILabel<NatronEngine.Param.setViewerUILabel>` (label)
*    def :meth:`setViewerUIVisible<NatronEngine.Param.setViewerUIVisible>` (visible)
*    def :meth:`linkTo<NatronEngine.Param.linkTo>` (otherParam[, thisDimension=-1, otherDimension=-1, thisView="All", otherView="All")
*    def :meth:`unlink<NatronEngine.Param.unlink>` ([dimension=-1,view="All"])

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

Note that most of the functions in the API of Params take a *dimension* parameter.
This is a 0-based index of the dimension on which to operate. For instance the dimension 0
of a RGB color parameter is the red value. 

Various properties controls the parameter regarding its animation or its layout or other
things.
Some properties are listed here, but the list is not complete. Refer to the reference on each
parameter type for all accessible properties.

	* addNewLine:	When True, the next parameter declared will be on the same line as this parameter
	
	* canAnimate: This is a static property that you cannot control which tells whether animation can be enabled for a specific type of parameter
	
	* animationEnabled: For all parameters that have canAnimate=True, this property controls whether this parameter should be able to animate (= have keyframes) or not
	
	* evaluateOnChange: This property controls whether a new render should be issued when the value of this parameter changes
	
	* help: This is the tooltip visible when hovering the parameter with the mouse
	
	* enabled: Should this parameter be editable by the user or not. Generally, disabled parameters have their text drawn in black.
	
	* visible: Should this parameter be visible in the user interface or not
	
	* persistent: If true then the parameter value will be saved in the project otherwise it will be forgotten between 2 runs
	
				 	

Note that  most of the properties are not dynamic and only work for user created parameters.
If calling any setter/getter associated to these properties, nothing will change right away.
A call to :func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>` is needed to refresh the GUI for these parameters.

For non *user-parameters* (i.e: parameters that were defined by the underlying OpenFX plug-in), only 
their **dynamic** properties can be changed since  :func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>`
will only refresh user parameters.
	
	
The following dynamic properties can be set on all parameters (non user and user):

+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| Name:             | Type:        |   Dynamic:   |         Setter:                | Getter:              | Default:              |
+===================+==============+==============+================================+======================+=======================+         
| visible           | bool         |   yes        |         setVisible             | getIsVisible         | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+
| enabled           | bool         |   yes        |         setEnabled             | getIsEnabled         | True                  |
+-------------------+--------------+--------------+--------------------------------+----------------------+-----------------------+


   
	.. note::
	
	 animates is set to True by default only if it is one of the following parameters:
    IntParam Int2DParam Int3DParam
    DoubleParam Double2DParam Double3DParam
    ColorParam
    
    Note that ParametricParam , GroupParam, PageParam, ButtonParam, FileParam, OutputFileParam,
    PathParam cannot animate at all.

	
Parameter in-viewer interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In Natron, each :ref:`Effect<Effect>` may have an interface in the Viewer, like the Roto or Tracker
nodes have.

You may add parameters on the viewer UI for any Effect as well as edit it. This also apply
to the Viewer node UI as well, so one can completely customize the Viewer toolbars. The user
 guide covers in detail how to customize the Viewer UI for an Effect.

To add a parameter to the Viewer UI of an Effect, use the function :func:`insertParamInViewerUI(parameter, index)<NatronEngine.Effect.insertParamInViewerUI>`.
You may then control its layout, using the :func:`setViewerUILayoutType(type)<NatronEngine.Param.setViewerUILayoutType>` function and the spacing
between parameters in pixels with :func:`setViewerUIItemSpacing(spacingPx)<NatronEngine.Param.setViewerUIItemSpacing>`.
You may set the text label or icon of the parameter specifically in the viewer UI by calling 
:func:`setViewerUIIconFilePath(filePath,checked)<NatronEngine.Param.setViewerUIIconFilePath>`
and :func:`setViewerUILabel(label)<NatronEngine.Param.setViewerUILabel>`.

	
Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Param.beginChanges()

    This can be called before making heavy changes to a parameter, such as setting thousands
    of keyframes. This call prevent the parameter from doing the following:
    - Trigger a new render when changed
    - Call the paramChanged callback when changed
    - Adjusting the folded/expanded state automatically for multi-dimensional parameters.
    
    Make sure to call the corresponding :func:`endChanges()<NatronEngine.Param.endChanges>`
    function when done

.. method:: NatronEngine.Param.copy(other [, dimension=-1])

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
	you might end-up with an undefined behaviour, e.g::
	
	If ChoiceParam1 has 3 entries and the current index is 2 and ChoiceParam2 has 15 entries
	and current index is 10, copying ChoiceParam2 to ChoiceParam1 will end-up in undefined behaviour.
	

This function returns **True** upon success and **False** otherwise.


.. method:: NatronEngine.Param.curve(time [, dimension=-1, view="Main"])

	:param time: :class:`float<PySide.QtCore.float>`
	:param dimension: :class:`int`
	:param view: :class:`str<PySide.QtCore.QString>`
	:rtype: :class:`float<PySide.QtCore.float>`
	
	If this parameter has an animation curve on the given *dimension*, then the value of
	that curve at the given *time* is returned. If the parameter has an expression on top
	of the animation curve, the expression will be ignored, ie.g: the value of the animation
	curve will still be returned. 
	This is useful to write custom expressions for motion design such as looping, reversing, etc...
	
.. method:: NatronEngine.Param.endChanges()

    To be called when finished making heavy changes to a parameter, such as setting thousands
    of keyframes. 

    A call to endChanges should always match a corresponding previous call to :func:`beginChanges()<NatronEngine.Param.beginChanges>`
    Note that they can be nested.
    
.. method:: NatronEngine.Param.getAddNewLine()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the parameter is on a new line or not.




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
	



.. method:: NatronEngine.Param.getIsEnabled()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether parameter is enabled or not.
When disabled the parameter cannot be edited from the user interface, however it can
still be edited from the Python A.P.I.



.. method:: NatronEngine.Param.getIsPersistent()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether this parameter should be persistent in the project or not.
Non-persistent parameter will not have their value saved when saving a project.




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

.. method:: NatronEngine.Param.getParentEffect()


    :rtype: :class:`NatronEngine.Effect`

	 If the holder of this parameter is an effect, this is the effect.
     If the holder of this parameter is a table item, this will return the effect holding the item
     itself.
     
    
.. method:: NatronEngine.Param.getParentItemBase()


    :rtype: :class:`NatronEngine.ItemBase`

	 If the holder of this parameter is a table item, this is the table item.
	 
	 
     
 .. method:: NatronEngine.Param.getApp()


    :rtype: :class:`NatronEngine.App`

	 If the holder of this parameter is the app itself (so it is a project setting), this is
     the app object.
     If the holder of this parameter is an effect, this is the application object containing
     the effect. 
     If the holder of this parameter is a table item, this will return the application
     containing the effect holding the item itself.

     
     
.. method:: NatronEngine.Param.getScriptName()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the *script-name* of the param as used internally. The script-name is visible
in the tooltip of the parameter when hovering the mouse over it on the GUI.
See :ref:`this section<autoVar>` for an explanation of the difference between
the *label* and the *script name*




.. method:: NatronEngine.Param.getTypeName()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the type-name of the parameter.

.. method:: NatronEngine.Param.getViewerUILayoutType ()

	:rtype: :class:`ViewerContextLayoutTypeEnum<NatronEngine.Natron.ViewerContextLayoutTypeEnum>`
	
	
	Returns the layout type of this parameter if it is present in the viewer interface of the Effect holding it.

.. method:: NatronEngine.Param.getViewerUIItemSpacing ()

	:rtype: :class:`int<PySide.QtCore.int>`	
	
	
	Returns the item spacing after this parameter if it is present in the viewer interface of the Effect holding it.
	
.. method:: NatronEngine.Param.getViewerUIIconFilePath ([checked=False])

	:param checked: :class:`bool<PySide.QtCore.bool>
	:rtype: :class:`str<NatronEngine.std::string>`
	
	Returns the icon file path of this parameter if it is present in the viewer interface of the Effect holding it.
	For buttons, if checked it false, the icon will be used when the button is unchecked, if checked it will be used
    when the button is checked.
	
.. method:: NatronEngine.Param.getHasViewerUI ()

	:rtype: :class:`bool<PySide.QtCore.bool>

	Returns whether this parameter has an interface in the Viewer UI of it's holding Effect.
	
.. method:: NatronEngine.Param.getViewerUIVisible ()

	:rtype: :class:`bool<PySide.QtCore.bool>

	Returns whether this parameter is visible in the Viewer UI. Only valid for parameters with a viewer ui
	
	
.. method:: NatronEngine.Param.getViewerUILabel ()

	:rtype: :class:`str<NatronEngine.std::string>`
	
	Returns the label of this parameter if it is present in the viewer interface of the Effect holding it.
	
		
.. method:: NatronEngine.Param.isExpressionCacheEnabled ()

	:rtype: :class:`bool<PySide.QtCore.bool>

	Returns whether caching of expression results is enabled for this knob.
	By default this is enabled, it can be disabled with :func:`setExpressionCacheEnabled(False)<NatronEngine.Param.setExpressionCacheEnabled>`
	
	
	
.. method:: NatronEngine.Param.random([min=0., max=1.])

	:param min: :class:`float<PySide.QtCore.float>`
	:param max: :class:`float<PySide.QtCore.float>`
	:rtype: :class:`float<PySide.QtCore.float>`

Returns a pseudo-random value in the interval [*min*, *max*[. 
The value is produced such that for a given parameter it will always be the same for a 
given time on the timeline, so that the value can be reproduced exactly.


.. note::

	Note that if you are calling multiple times random() in the same parameter expression,
	each call would return a different value, but they would all return the same value again
	if the expressions is interpreted at the same time, e.g::
	
		# Would always return the same value at a given timeline's time.
		random() - random() 
		
Note that you can ensure that random() returns a given value by calling the overloaded
function :func:`random(min,max,time,seed)<NatronEngine.Param.random>` instead.

.. method:: NatronEngine.Param.random(min, max, time, [seed=0])

	:param min: :class:`float<PySide.QtCore.float>`
	:param max: :class:`float<PySide.QtCore.float>`
	:param time: :class:`float<PySide.QtCore.float>`
	:param seed: :class:`unsigned int<PySide.QtCore.int>`
	:rtype: :class:`float<PySide.QtCore.float>`

Same as :func:`random()<NatronEngine.Param.random>` but takes **time** and **seed** in parameters to control
the value returned by the function. E.g::

	ret = random(0,1,frame,2) - random(0,1,frame,2)
	# ret == 0 always
	
.. method:: NatronEngine.Param.randomInt(min,max)

	:param min: :class:`int<PySide.QtCore.int>`
	:param max: :class:`int<PySide.QtCore.int>`
	:rtype: :class:`int<PySide.QtCore.int>`

Same as  :func:`random(min,max)<NatronEngine.Param.random>` but returns an integer in the 
range [*min*,*max*[

.. method:: NatronEngine.Param.randomInt(min, max, time, [seed=0])
	
	:param min: :class:`int<PySide.QtCore.int>`
	:param max: :class:`int<PySide.QtCore.int>`
	:param time: :class:`float<PySide.QtCore.float>`
	:param seed: :class:`unsigned int<PySide.QtCore.int>`
	:rtype: :class:`int<PySide.QtCore.int>`	
	
Same as :func:`random(min,max,time,seed)<NatronEngine.Param.random>` but returns an integer in the range
[0, INT_MAX] instead. 


.. method:: NatronEngine.Param.setAddNewLine(a)


    :param a: :class:`bool<PySide.QtCore.bool>`

Set whether the parameter should be on a new line or not. 
See :func:`getAddNewLine()<NatronEngine.Param.getAddNewLine>`




.. method:: NatronEngine.Param.setAnimationEnabled(e)


    :param e: :class:`bool<PySide.QtCore.bool>`

Set whether animation should be enabled (= can have keyframes). 
See :func:`getIsAnimationEnabled()<NatronEngine.Param.getIsAnimationEnabled>`




.. method:: NatronEngine.Param.setEnabled(enabled)


    :param enabled: :class:`bool<PySide.QtCore.bool>`

Set whether the parameter should be enabled or not.
When disabled, the parameter will be displayed in black and the user will not be able
to edit it.
See :func:`getIsEnabled(dimension)<NatronEngine.Param.getIsEnabled>`


.. method:: NatronEngine.Param.setEvaluateOnChange(eval)


    :param eval: :class:`bool<PySide.QtCore.bool>`

Set whether evaluation should be enabled for this parameter. When True, calling any
function that change the value of the parameter will trigger a new render.
See :func:`getEvaluateOnChange()<NatronEngine.Param.getEvaluateOnChange>`


.. method:: NatronEngine.Param.setIconFilePath(icon [,checked])


    :param icon: :class:`str<NatronEngine.std::string>`
    :param checked: :class:`bool<PySide.QtCore.bool>`

Set here the icon file path for the label. This should be either an absolute path or
a file-path relative to a path in the NATRON_PLUGIN_PATH. The icon will replace the
label of the parameter. If this parameter is a :ref:`ButtonParam<ButtonParam>` then
if *checked* is *True* the icon will be used when the button is down. Similarily if
*checked* is *False* the icon will be used when the button is up.


.. method:: NatronEngine.Param.setLabel(label)


    :param label: :class:`str<NatronEngine.std::string>`

Set the label of the parameter as visible in the GUI
See :func:`getLabel()<NatronEngine.Param.getLabel>`



.. method:: NatronEngine.Param.setHelp(help)


    :param help: :class:`str<NatronEngine.std::string>`

Set the help tooltip of the parameter.
See :func:`getHelp()<NatronEngine.Param.getHelp>`


.. method:: NatronEngine.Param.setPersistent(persistent)


    :param persistent: :class:`bool<PySide.QtCore.bool>`

Set whether this parameter should be persistent or not.
Non persistent parameter will not be saved in the project.
See :func:`getIsPersistent<NatronEngine.Param.getIsPersistent>`




.. method:: NatronEngine.Param.setVisible(visible)


    :param visible: :class:`bool<PySide.QtCore.bool>`
	
Set whether this parameter should be visible or not to the user.
See :func:`getIsVisible()<NatronEngine.Param.getIsVisible>`


.. method:: NatronEngine.Param.setViewerUILayoutType (type)

	:param type: :class:`NatronEngine.Natron.ViewerContextLayoutTypeEnum<NatronEngine.Natron.ViewerContextLayoutTypeEnum>`
	
	
	Set the layout type of this parameter if it is present in the viewer interface of the Effect holding it.

.. method:: NatronEngine.Param.setViewerUIItemSpacing (spacing)

	:param spacing: :class:`int<PySide.QtCore.int>`	
	
	
	Set the item spacing after this parameter if it is present in the viewer interface of the Effect holding it.
	
.. method:: NatronEngine.Param.setViewerUIIconFilePath (filePath[,checked=False])


	:param filePath: :class:`str<NatronEngine.std::string>`
	:param checked: :class:`bool<PySide.QtCore.bool>`
	
	Set the icon file path of this parameter if it is present in the viewer interface of the Effect holding it.
	For buttons, if checked it false, the icon will be used when the button is unchecked, if checked it will be used
    when the button is checked.
    This function only has an effect on user created parameters.
	
	
.. method:: NatronEngine.Param.setViewerUILabel (label)

	:param label: :class:`str<NatronEngine.std::string>`
	
	Set the label of this parameter if it is present in the viewer interface of the Effect holding it.
	This function only has an effect on user created parameters.
	
	
.. method:: NatronEngine.Param.setViewerUIVisible (visible)

	:param visible: :class:`bool<PySide.QtCore.bool>`
	
	Set this parameter visible or not in the Viewer UI. Only valid for parameters for which
	the function :func:`getHasViewerUI()<NatronEngine.Param.getHasViewerUI>` returns *True*.

	
.. method:: NatronEngine.Param.setExpressionCacheEnabled (enabled)

	:param enabled: :class:`bool<PySide.QtCore.bool>`
	
	Set whether caching of expression results is enabled. By default this is True.
	This can be turned off if an expression is set on a parameter but the expression depends
	on external data (other than parameter values, such as a file on disk).
	These external data would be unknown from Natron hence the expression cache would never
	invalidate.


.. method:: NatronEngine.Param.linkTo(otherParam[, thisDimension=-1, otherDimension=-1,thisView="All",otherView="All"])

	:param otherParam: :class:`Param<NatronEngine.Param>`
	:param thisDimension: :class:`int<PySide.QtCore.int>`
	:param otherDimension: :class:`int<PySide.QtCore.int>`
	:param thisView: :class:`str<PySide.QtCore.QString>`
	:param otherView: :class:`str<PySide.QtCore.QString>`
	:rtype: :class:`bool<PySide.QtCore.bool>`	
	
This parameter will share the value of *otherParam*. 
They need to be both of the same *type* but may vary in dimension, as long as 
*thisDimension* is valid according to the number of dimensions of this parameter and 
*otherDimension* is valid according to the number of dimensions of *otherParam*.
If *thisDimension* is -1 then it is expected that *otherDimension* is also -1 indicating
that all dimensions should respectively be slaved.

If this parameter has split views, then only view(s) specified by *thisView* will be slaved
to the *otherView* of the other parameter.
If *thisView* is "All" then it is expected that *otherView* is also "All" indicating that all
views should be respectively slaved. If not set to "All" then the view parameters should 
name valid views in the project settings.


This parameter *thisDimension* will be controlled entirely by the *otherDimension* of
*otherParam* until a call to :func:`unlink(thisDimension)<NatronEngine.Param.unlink>` is made

.. method:: NatronEngine.Param.unlink([dimension=-1,view="All"])

	:param dimension: :class:`int<PySide.QtCore.int>`
	:param view: :class:`str<PySide.QtCore.QString>`

If the given *dimension* of this parameter was previously linked, then this function will
remove the link and the value will no longer be shared with any other parameters.
If *dimension* equals -1 then all dimensions will be unlinked.
If *view* is set to "All" then all views will be unlinked, otherwise it should 
name valid views in the project settings.

.. note::

	 The animation and values that were present before the link will remain.
