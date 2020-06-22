.. module:: NatronEngine
.. _AnimatedParam:

AnimatedParam
*************

**Inherits** :doc:`Param`

**Inherited by:** :doc:`StringParamBase`, :doc:`PathParam`, :doc:`OutputFileParam`, :doc:`FileParam`, :doc:`StringParam`, :doc:`BooleanParam`, :doc:`ChoiceParam`, :doc:`ColorParam`, :doc:`DoubleParam`, :doc:`Double2DParam`, :doc:`Double3DParam`, :doc:`IntParam`, :doc:`Int2DParam`, :doc:`Int3DParam`

Synopsis
--------

This is the base class for all parameters which have the property :func:`canAnimate<NatronEngine.Param.getCanAnimate>` set to True.
See the :ref:`detailed description<details>` below

Functions
^^^^^^^^^

- def :meth:`deleteValueAtTime<NatronEngine.AnimatedParam.deleteValueAtTime>` (time[, dimension=0])
- def :meth:`getCurrentTime<NatronEngine.AnimatedParam.getCurrentTime>` ()
- def :meth:`getDerivativeAtTime<NatronEngine.AnimatedParam.getDerivativeAtTime>` (time[, dimension=0])
- def :meth:`getExpression<NatronEngine.AnimatedParam.getExpression>` (dimension)
- def :meth:`getIntegrateFromTimeToTime<NatronEngine.AnimatedParam.getIntegrateFromTimeToTime>` (time1, time2[, dimension=0])
- def :meth:`getIsAnimated<NatronEngine.AnimatedParam.getIsAnimated>` ([dimension=0])
- def :meth:`getKeyIndex<NatronEngine.AnimatedParam.getKeyIndex>` (time[, dimension=0])
- def :meth:`getKeyTime<NatronEngine.AnimatedParam.getKeyTime>` (index, dimension)
- def :meth:`getNumKeys<NatronEngine.AnimatedParam.getNumKeys>` ([dimension=0])
- def :meth:`removeAnimation<NatronEngine.AnimatedParam.removeAnimation>` ([dimension=0])
- def :meth:`setExpression<NatronEngine.AnimatedParam.setExpression>` (expr, hasRetVariable[, dimension=0])
- def :meth:`setInterpolationAtTime<NatronEngine.AnimatedParam.setInterpolationAtTime>` (time, interpolation[, dimension=0])

.. _details:

Detailed Description
--------------------

Animating parameters have values that may change throughout the time. To enable animation
the parameter should have at least 1 keyframe. Keyframes can be added in the derived class
(since function signature is type specific) with the *setValueAtTime* function.
Once 2 keyframes are active on the parameter, the value of the parameter will be interpolated
automatically by Natron for a given time.
You can control keyframes by adding,removing, changing their values and their :class:`KeyFrameTypeEnum<NatronEngine.Natron.KeyframeTypeEnum>` type.

Note that by default new keyframes are always with a **Smooth** interpolation.

Moreover parameters can have Python expressions set on them to control their value. In that case, the expression takes
precedence over any animation that the parameter may have, meaning that the value of the parameter would be computed
using the expression provided.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: NatronEngine.AnimatedParam.deleteValueAtTime(time[, dimension=0])


    :param time: :class:`float<PySide.QtCore.float>`
    :param dimension: :class:`int<PySide.QtCore.int>`

Removes a keyframe at the given *time* and *dimension* for this parameter, if such
keyframe exists.




.. method:: NatronEngine.AnimatedParam.getCurrentTime()


    :rtype: :class:`int<PySide.QtCore.int>`

Convenience function: returns the current time on the timeline




.. method:: NatronEngine.AnimatedParam.getDerivativeAtTime(time[, dimension=0])


    :param time: :class:`float<PySide.QtCore.double>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`double<PySide.QtCore.double>`

Returns the derivative of the parameter at the given *time* and for the given
*dimension*. The derivative is computed on the animation curve of the parameter.
This function is irrelevant for parameters that have an expression.




.. method:: NatronEngine.AnimatedParam.getExpression(dimension)


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`str<NatronEngine.std::string>`

Returns the Python expression set on the parameter at the given dimension.
When no expression is set, this function returns an empty string.



.. method:: NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(time1, time2[, dimension=0])


    :param time1: :class:`float<PySide.QtCore.double>`
    :param time2: :class:`float<PySide.QtCore.double>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`float<PySide.QtCore.double>`

Integrates the value of the parameter over the range [*time1* - *time2*].
This is done using the animation curve of the parameter of the given *dimension*.
Note that if this parameter has an expression, the return value is irrelevant.



.. method:: NatronEngine.AnimatedParam.getIsAnimated([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the given *dimension* has an animation or not.
This returns true if the underlying animation curve has 1 or more keyframes.




.. method:: NatronEngine.AnimatedParam.getKeyIndex(time[, dimension=0])


    :param time: :class:`float<PySide.QtCore.float>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the index of the keyframe at the given *time* for the animation curve
at the given *dimension*, or -1 if no such keyframe could be found.




.. method:: NatronEngine.AnimatedParam.getKeyTime(index, dimension)


    :param index: :class:`int<PySide.QtCore.int>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`tuple`

Returns a tuple [bool,float] where the first member is True if a keyframe exists at
the given *index* for the animation curve at the given *dimension*.
The second *float* member is the keyframe exact time.





.. method:: NatronEngine.AnimatedParam.getNumKeys([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of keyframes for the animation curve at the given *dimension*.




.. method:: NatronEngine.AnimatedParam.removeAnimation([dimension=0])


    :param dimension: :class:`int<PySide.QtCore.int>`

Removes all animation for the animation curve at the given *dimension*.
Note that this will not remove any expression set.




.. method:: NatronEngine.AnimatedParam.setExpression(expr, hasRetVariable[, dimension=0])


    :param expr: :class:`str<NatronEngine.std::string>`
    :param hasRetVariable: :class:`bool<PySide.QtCore.bool>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Set the Python expression *expr* on the parameter at the given *dimension*
If *hasRetVariable* is True, then *expr* is assumed to have a variable *ret* declared.
Otherwise, Natron will declare the *ret* variable itself.

.. method:: NatronEngine.AnimatedParam.setInterpolationAtTime(time, interpolation[, dimension=0])

    :param time: :class:`float<PySide.QtCore.float>`
    :param interpolation: :class:`KeyFrameTypeEnum<NatronEngine.KeyFrameTypeEnum>`
    :param dimension: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Set the interpolation of the animation curve of the given dimension at the given keyframe.
If no such keyframe could be found, this method returns False.
Upon success, this method returns True.

Example::

    app1.Blur2.size.setInterpolationAtTime(56,NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeConstant,0)
