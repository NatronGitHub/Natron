.. currentmodule:: NatronEngine
.. _SeparatorParam:

SeparatorParam
**************

**Inherits** :doc:`Param`

Synopsis
--------

A separator is a horizontal line drawn in the settings panel of a node, used to
visually separate groups of parameters. It holds no value and cannot be animated.

Create one on a node or a Group with
:func:`createSeparatorParam(name,label)<NatronEngine.UserParamHolder.createSeparatorParam>`::

    sep = group.createSeparatorParam("sep1", "")
    group.refreshUserParamsGUI()

A separator has no functions of its own; it only has the functions inherited from :doc:`Param`.
