.. module:: NatronEngine
.. _AnimatedParam:

AnimatedParam
*************

**Inherits** :doc:`Param`

**Inherited by:** :ref:`StringParamBase`, :ref:`PathParam`, :ref:`OutputFileParam`, :ref:`FileParam`, :ref:`StringParam`, :ref:`BooleanParam`, :ref:`ChoiceParam`, :ref:`ColorParam`, :ref:`DoubleParam`, :ref:`Double2DParam`, :ref:`Double3DParam`, :ref:`IntParam`, :ref:`Int2DParam`, :ref:`Int3DParam`

Synopsis
--------

This is the base class for all parameters which have the property :func:`canAnimate<NatronEngine.Param.getCanAnimate>` set to True.
See the :ref:`detailed description<details>` below

Functions
^^^^^^^^^

*    def :meth:`deleteValueAtTime<NatronEngine.AnimatedParam.deleteValueAtTime>` (time[, dimension=0,view="All"])
*    def :meth:`getCurrentTime<NatronEngine.AnimatedParam.getCurrentTime>` ()
*    def :meth:`getDerivativeAtTime<NatronEngine.AnimatedParam.getDerivativeAtTime>` (time[, dimension=0,view="Main"])
*    def :meth:`getExpression<NatronEngine.AnimatedParam.getExpression>` (dimension[,view="Main"])
*    def :meth:`getIntegrateFromTimeToTime<NatronEngine.AnimatedParam.getIntegrateFromTimeToTime>` (time1, time2[, dimension=0,view="Main"])
*    def :meth:`getIsAnimated<NatronEngine.AnimatedParam.getIsAnimated>` ([dimension=0,view="Main"])
*    def :meth:`getKeyIndex<NatronEngine.AnimatedParam.getKeyIndex>` (time[, dimension=0,view="Main"])
*    def :meth:`getKeyTime<NatronEngine.AnimatedParam.getKeyTime>` (index, dimension[, view="Main"])
*    def :meth:`getNumKeys<NatronEngine.AnimatedParam.getNumKeys>` ([dimension=0,view="Main"])
*    def :meth:`removeAnimation<NatronEngine.AnimatedParam.removeAnimation>` ([dimension-1, view="All"])
*    def :meth:`setExpression<NatronEngine.AnimatedParam.setExpression>` (expr, hasRetVariable[, dimension=-1,view="All"])
*    def :meth:`setInterpolationAtTime<NatronEngine.AnimatedParam.setInterpolationAtTime>` (time, interpolation[, dimension=-1,view="All"])
*	 def :meth:`splitView<NatronEngine.AnimatedParam.splitView>` (view)
*	 def :meth:`unSplitView<NatronEngine.AnimatedParam.unSplitView>` (view)
*	 def :meth:`getViewsList<NatronEngine.AnimatedParam.getViewsList>` ()

.. _details:

Detailed Description
--------------------

Animating parameters have values that may change throughout the time. To enable animation
the parameter should have at least 1 keyframe. Keyframes can be added in the derived class
(since function signature is type specific) with the *setValueAtTime* function.
Once 2 keyframes are active on the parameter, the value of the parameter will be interpolated
automatically by Natron for a given time.
You can control keyframes by adding,removing, changing their values and their :ref:`interpolation<NatronEngine.Natron.KeyframeTypeEnum>` type.

Note that by default new keyframes are always with a **Smooth** interpolation.

Moreover parameters can have Python expressions set on them to control their value. In that case, the expression takes
precedence over any animation that the parameter may have, meaning that the value of the parameter would be computed
using the expression provided.

Most of the functions to modify the value of the parameter take in parameter a *view* parameter.
See :ref:`this<multiViewParams>` section for more informations.

Example::

	# We assume the project has 2 views: the first named "Left" and the other named "Right"

	# X and Y of the size parameter of the blur have now the value 3
	Blur1.size.set(3,3)

	# We split-off the "Right" view
	Blur1.size.splitView("Right")

	Blur1.size.set(5,5,"Right")

	# The left view still has (3,3) but the right view now has (5,5)

	Blur1.size.unSplitView("Right")

	# Imagine now the project has 3 views: "Left" "Right" "Center"
	# Only the "Right" view is split-off

	# Setting the "Main" view will set "Left" + "Center"
	Blur1.size.set(2,2, "Main")

	# Note that this is the same as calling the following
	# since by default all views that are not split off
	# have the same value as the first view
	Blur1.size.set(2,2, "Left")

	# Calling the following will print an error
	# because the view is not split
	Blur1.size.set(2,2, "Center")

	# The following call will set all views at once
	Blur1.size.set(10,10, "All")




Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: NatronEngine.AnimatedParam.deleteValueAtTime(time[, dimension=0,view="All"])


    :param time: :class:`float<PySide.QtCore.float>`
    :param dimension: :class:`int<PySide.QtCore.int>`
   	:param view: :class:`str<PySide.QtCore.QString>`

Removes a keyframe at the given *time*, *dimension* and *view* for this parameter, if such
keyframe exists.




.. method:: NatronEngine.AnimatedParam.getCurrentTime()


    :rtype: :class:`int<PySide.QtCore.int>`

Convenience function: returns the current time on the timeline




.. method:: NatronEngine.AnimatedParam.getDerivativeAtTime(time[, dimension=0,view="Main"])


    :param time: :class:`float<PySide.QtCore.double>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`double<PySide.QtCore.double>`

Returns the derivative of the parameter at the given *time* and for the given
*dimension* and *view*. The derivative is computed on the animation curve of the parameter.
This function is irrelevant for parameters that have an expression.




.. method:: NatronEngine.AnimatedParam.getExpression(dimension,[view="Main"])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`str<NatronEngine.std::string>`

Returns the Python expression set on the parameter at the given *dimension* and *view*.
When no expression is set, this function returns an empty string.



.. method:: NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(time1, time2[, dimension=0,view="Main"])


    :param time1: :class:`float<PySide.QtCore.double>`
    :param time2: :class:`float<PySide.QtCore.double>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`float<PySide.QtCore.double>`

Integrates the value of the parameter over the range [*time1* - *time2*].
This is done using the animation curve of the parameter of the given *dimension* and *view*.
Note that if this parameter has an expression, the return value is irrelevant.



.. method:: NatronEngine.AnimatedParam.getIsAnimated([dimension=0,view="Main"])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the given *dimension* and *view* has an animation or not.
This returns true if the underlying animation curve has 1 or more keyframes.




.. method:: NatronEngine.AnimatedParam.getKeyIndex(time[, dimension=0,view="Main"])


    :param time: :class:`float<PySide.QtCore.float>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the index of the keyframe at the given *time* for the animation curve
at the given *dimension*, or -1 if no such keyframe could be found.




.. method:: NatronEngine.AnimatedParam.getKeyTime(index, dimension[,view="Main"])


    :param index: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`tuple`

Returns a tuple [bool,float] where the first member is True if a keyframe exists at
the given *index* for the animation curve at the given *dimension* and *view*.
The second *float* member is the keyframe exact time.





.. method:: NatronEngine.AnimatedParam.getNumKeys([dimension=0,view="Main"])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of keyframes for the animation curve at the given *dimension* and *view*.




.. method:: NatronEngine.AnimatedParam.removeAnimation([dimension=-1,view="All"])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`

Removes all animation for the animation curve at the given *dimension* and *view*.
Note that this will not remove any expression set.




.. method:: NatronEngine.AnimatedParam.setExpression(expr, hasRetVariable[, dimension=-1, view="All"])


    :param expr: :class:`str<NatronEngine.std::string>`
    :param hasRetVariable: :class:`bool<PySide.QtCore.bool>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Set the Python expression *expr* on the parameter at the given *dimension* and *view*.
If *hasRetVariable* is True, then *expr* is assumed to have a variable *ret* declared.
Otherwise, Natron will declare the *ret* variable itself.

.. method:: NatronEngine.AnimatedParam.setInterpolationAtTime(time, interpolation[, dimension=-1,view="All"])

    :param time: :class:`float<PySide.QtCore.float>`
    :param interpolation: :class:`KeyFrameTypeEnum<NatronEngine.KeyFrameTypeEnum>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Set the interpolation of the animation curve of the given *dimension* and *view* at the given keyframe *time*.
If no such keyframe could be found, this method returns False.
Upon success, this method returns True.

Example::

    app1.Blur2.size.setInterpolationAtTime(56,NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeConstant,0)

.. method:: NatronEngine.AnimatedParam.splitView (view)

    :param view: :class:`view<PySide.QtCore.QString>`

Split-off the given *view* in the parameter so that it can be assigned different a value
and animation than the *Main* view.
See :ref:`the section on multi-view<multiViewParams>` for more infos.

.. method:: NatronEngine.AnimatedParam.unSplitView (view)

If the given *view* was previously split off by a call to :func:`splitView(view)<NatronEngine.AnimatedParam.splitView>`
then the view-specific values and animation will be removed and all subsequent access
to these values will return the value of the *Main* view.
See :ref:`the section on multi-view<multiViewParams>` for more infos.

.. method:: NatronEngine.AnimatedParam.getViewsList ()


Returns a list of all views that have a different value in the parameter. All views
of the project that do not appear in this list are considered to be the same as
the first view returned by this function.
