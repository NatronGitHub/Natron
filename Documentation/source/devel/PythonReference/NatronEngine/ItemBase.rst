.. module:: NatronEngine
.. _ItemBase:

ItemBase
********


**Inherited by:** :doc:`BezierCurve`, :doc:`Layer`

Synopsis
--------

This is an abstract class that serves as a base class for both :doc:`Layer` and :doc:`BezierCurve`.
See :ref:`detailed<itemBase.details>` description...

Functions
^^^^^^^^^

- def :meth:`getLabel<NatronEngine.ItemBase.getLabel>` ()
- def :meth:`getLocked<NatronEngine.ItemBase.getLocked>` ()
- def :meth:`getLockedRecursive<NatronEngine.ItemBase.getLockedRecursive>` ()
- def :meth:`getParentLayer<NatronEngine.ItemBase.getParentLayer>` ()
- def :meth:`getParam<NatronEngine.ItemBase.getParam>` (name)
- def :meth:`getScriptName<NatronEngine.ItemBase.getScriptName>` ()
- def :meth:`getVisible<NatronEngine.ItemBase.getVisible>` ()
- def :meth:`setLabel<NatronEngine.ItemBase.setLabel>` (name)
- def :meth:`setLocked<NatronEngine.ItemBase.setLocked>` (locked)
- def :meth:`setScriptName<NatronEngine.ItemBase.setScriptName>` (name)
- def :meth:`setVisible<NatronEngine.ItemBase.setVisible>` (activated)

.. _itemBase.details:

Detailed Description
--------------------

This class gathers all common functions to both :doc:`layers<Layer>` and :doc:`beziers<BezierCurve>`.
An item has both a *script-name* and *label*. The *script-name* uniquely identifies an item
within a roto node, while several items can have the same *label*.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.ItemBase.getLabel()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the label of the item, has visible in the table of the settings panel.




.. method:: NatronEngine.ItemBase.getLocked()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether this item is locked or not. When locked the item is no longer editable by
the user.




.. method:: NatronEngine.ItemBase.getLockedRecursive()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether this item is locked or not. Unlike :func:`getLocked()<NatronEngine.ItemBase.getLocked>`
this function looks parent layers recursively to find out if the item should be locked.




.. method:: NatronEngine.ItemBase.getParentLayer()


    :rtype: :class:`Layer<NatronEngine.Layer>`

Returns the parent :doc:`layer<Layer>` of the item. All items must have a parent layer,
except the base layer.

.. method:: NatronEngine.ItemBase.getParam(name)


    :param name: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Param<Param>`


Returns a :doc:`parameter<Param>` by its script-name or None if
no such parameter exists.



.. method:: NatronEngine.ItemBase.getScriptName()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the *script-name* of the item. The script-name is unique for each items in a roto
node.




.. method:: NatronEngine.ItemBase.getVisible()


    :rtype: :class:`bool<PySide.QtCore.bool>`

Returns whether the item is visible or not. On the user interface, this corresponds to the
small *eye*. When hidden, an item will no longer have its overlay painted on the viewer,
but it will still render in the image.




.. method:: NatronEngine.ItemBase.setLabel(name)


    :param name: :class:`str<NatronEngine.std::string>`

Set the item's label.




.. method:: NatronEngine.ItemBase.setLocked(locked)


    :param locked: :class:`bool<PySide.QtCore.bool>`

Set whether the item should be locked or not. See :func:`getLocked()<NatronEngine.ItemBase.getLocked>`.




.. method:: NatronEngine.ItemBase.setScriptName(name)


    :param name: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Set the script-name of the item. You should never call it yourself as Natron chooses 
automatically a unique script-name for each item. However this function is made available
for internal technicalities, but be aware that changing the script-name of an item
can potentially break other scripts relying on it.




.. method:: NatronEngine.ItemBase.setVisible(activated)


    :param activated: :class:`bool<PySide.QtCore.bool>`

Set whether the item should be visible in the Viewer. See :func:`getVisible()<NatronEngine.ItemBase.getVisible>`.





