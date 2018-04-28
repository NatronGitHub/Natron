.. module:: NatronEngine
.. _GroupParam:

GroupParam
**********

**Inherits** :doc:`Param`

Synopsis
--------

A group param is a container for other parameters.
See :ref:`detailed<groupparams.details>` description.

Functions
^^^^^^^^^

- def :meth:`addParam<NatronEngine.GroupParam.addParam>` (param)
- def :meth:`getIsOpened<NatronEngine.GroupParam.getIsOpened>` ()
- def :meth:`setAsTab<NatronEngine.GroupParam.setAsTab>` ()
- def :meth:`setOpened<NatronEngine.GroupParam.setOpened>` (opened)

.. _groupparams.details:

Detailed Description
--------------------

A group param does not hold any relevant value. Rather this is a purely graphical
element that is used to gather multiple parameters under a group.
On the graphical interface a GroupParam looks like this:

.. figure:: groupParam.png


When a :doc:`Param` is under a group, the :func:`getParent()<NatronEngine.Param.getParent>`
will return the group as parent.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.GroupParam.addParam(param)


    :param param: :class:`Param<NatronEngine.Param>`


Adds *param* into the group.


.. warning::

    Note that this function cannot be called on groups that are not user parameters (i.e: created
    either by script or by the "Manage user parameters" user interface)


.. warning::

    Once called, you should call :func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>`
    to update the user interface.

.. method:: NatronEngine.GroupParam.getIsOpened()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the group is currently expanded (True) or folded (False).




.. method:: NatronEngine.GroupParam.setAsTab()


Set this group as a tab. When set as a tab, it will be inserted into a special TabWidget
of the Effect.
For instance, on the following screenshot, *to* and *from* are 2 groups on which
:func:`setAsTab()<NatronEngine.GroupParam.setAsTab>` has been called.

.. figure:: groupAsTab.png


.. method:: NatronEngine.GroupParam.setOpened(opened)


    :param opened: :class:`bool<PySide.QtCore.bool>`

Set this group to be expanded (*opened* = True) or folded (*opened* = False)





