.. module:: NatronEngine
.. _ItemBase:

ItemBase
********

**Inherited by:** :ref:`BezierCurve<BezierCurve>`, :ref:`StrokeItem<StrokeItem>`, :ref:`Track<Track>`

Synopsis
--------

This is an abstract class that serves as a base class for all items that can be inserted
in a :ref`ItemsTable<ItemsTable>`

See :ref:`detailed<itemBase.details>` description...

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`deleteUserKeyframe<NatronEngine.ItemBase.deleteUserKeyframe>` (time[,view="All"])
*    def :meth:`getLabel<NatronEngine.ItemBase.getLabel>` ()
*    def :meth:`getIconFilePath<NatronEngine.ItemBase.getIconFilePath>` ()
*    def :meth:`getParent<NatronEngine.ItemBase.getParent>` ()
*    def :meth:`getIndexInParent<NatronEngine.ItemBase.getIndexInParent>` ()
*    def :meth:`getChildren<NatronEngine.ItemBase.getChildren>` ()
*    def :meth:`getParam<NatronEngine.ItemBase.getParam>` (name)
*    def :meth:`getParams<NatronEngine.ItemBase.getParams>` ()
*    def :meth:`getUserKeyframes<NatronEngine.ItemBase.getUserKeyframes>` ([view="Main"])
*    def :meth:`getScriptName<NatronEngine.ItemBase.getScriptName>` ()
*    def :meth:`setLabel<NatronEngine.ItemBase.setLabel>` (name)
*    def :meth:`setIconFilePath<NatronEngine.ItemBase.setIconFilePath>` (name)
*    def :meth:`setUserKeyframe<NatronEngine.ItemBase.setUserKeyframe>` ([view="All"])


.. _itemBase.details:

Detailed Description
--------------------

This class gathers all common functions to both :doc:`layers<Layer>` and :doc:`beziers<BezierCurve>`.
An item has both a *script-name* and *label*. The *script-name* uniquely identifies an item
within a roto node, while several items can have the same *label*.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.ItemBase.deleteUserKeyframe(time [, view="All"])

    :param time: :class:`double<PySide.QtCore.double>`
    :param view: :class:`str<PySide.QtCore.QString>`

    Removes any user keyframe set at the given timeline *time* and for the given *view*.


.. method:: NatronEngine.ItemBase.getLabel()


    :rtype: :class:`str<PySide.QtCore.QString>`

Returns the label of the item, as visible in the table of the settings panel.


.. method:: NatronEngine.ItemBase.getIconFilePath()


    :rtype: :class:`str<PySide.QtCore.QString>`

Returns the icon file path of the item, as visible in the table of the settings panel.



.. method:: NatronEngine.ItemBase.getParent()


    :rtype: :class:`ItemBase<NatronEngine.ItemBase>`

Returns the parent :ref:`item<ItemBase>` of the item if it has one. For :ref:`ItemsTable<ItemsTable>`
that have their function :func:`isModelParentingEnabled()<NatronEngine.ItemsTable.isModelParentingEnabled>`
returning *False* this function will always return *None*.


.. method:: NatronEngine.ItemBase.getIndexInParent()


    :rtype: :class:`int<PySide.QtCore.QString>`

If this item has a parent, returns the index of this item in the parent's children list.
If this item is a top-level item, returns the index of this item in the model top level items list.
This function returns -1 if the item is not in a model.


.. method:: NatronEngine.ItemBase.getChildren()


    :rtype: :class:`Sequence`

Returns a list of children :ref:`items<ItemBase>`. For :ref:`ItemsTable<ItemsTable>`
that have their function :func:`isModelParentingEnabled()<NatronEngine.ItemsTable.isModelParentingEnabled>`
returning *False* this function will always return an empty sequence.


.. method:: NatronEngine.ItemBase.getParam(name)


    :param name: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`Param<Param>`


Returns a :doc:`parameter<Param>` by its script-name or None if
no such parameter exists.

.. method:: NatronEngine.ItemBase.getParams()

    :rtype: :class:`Sequence`


Returns a list of all :doc:`parameters<Param>` held by the item.


.. method:: NatronEngine.ItemBase.getUserKeyframes([view="Main"])

    :param view: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`Sequence`

Return a list of list of double with all user keyframe times on the timeline for the given *view*.


.. method:: NatronEngine.ItemBase.getScriptName()


    :rtype: :class:`str<PySide.QtCore.QString>`

Returns the *script-name* of the item. The script-name is unique for each items in a roto
node.


.. method:: NatronEngine.ItemBase.setLabel(name)


    :param name: :class:`str<PySide.QtCore.QString>`

Set the item's label.


.. method:: NatronEngine.ItemBase.setIconFilePath(icon)


    :param icon: :class:`str<PySide.QtCore.QString>`

Set the item's icon file path.



.. method:: NatronEngine.ItemBase.setUserKeyframe(time [, view="All"])

    :param time: :class:`double<PySide.QtCore.double>`
    :param view: :class:`str<PySide.QtCore.QString>`

    Set a user keyframe at the given timeline *time* and for the given *view*.




