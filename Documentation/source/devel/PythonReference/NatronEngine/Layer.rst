.. currentmodule:: NatronEngine
.. _Layer:

Layer
*****

**Inherits** :doc:`ItemBase`

Synopsis
--------

This class is used to group several shapes together and to organize them so they are 
rendered in a specific order.
See :ref:`detailed<layer.details>` description...

Functions
^^^^^^^^^

- def :meth:`addItem<NatronEngine.Layer.addItem>` (item)
- def :meth:`getChildren<NatronEngine.Layer.getChildren>` ()
- def :meth:`insertItem<NatronEngine.Layer.insertItem>` (pos, item)
- def :meth:`removeItem<NatronEngine.Layer.removeItem>` (item)

.. _layer.details:

Detailed Description
--------------------

Currently a layer acts only as a group so that you can organize shapes and control in 
which order they are rendered. 
To add a new :doc:`item<ItemBase>` to the layer, use the :func:`addItem(item)<NatronEngine.Layer.addItem>` or
the :func:`insertItem(item)<NatronEngine.Layer.insertItem>` function.

To remove an item from the layer, use the :func:`removeItem(item)<NatronEngine.Layer.removeItem>` function.

Items in a layer are rendered from top to bottom, meaning the bottom-most items will always
be drawn on top of other items. 

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: Layer.addItem(item)


    :param item: :class:`ItemBase<NatronEngine.ItemBase>`


Adds a new item at the bottom of the layer.



.. method:: Layer.getChildren()


    :rtype: :class:`sequence`

Returns a sequence with all :doc:`items<ItemBase>` in the layer.




.. method:: Layer.insertItem(pos, item)


    :param pos: :class:`int`
    :param item: :class:`ItemBase<NatronEngine.ItemBase>`


Inserts a new item at the given *pos* (0 based index) in the layer. If *pos* is out of range,
it will be inserted at the bottom of the layer.



.. method:: Layer.removeItem(item)


    :param item: :class:`ItemBase<NatronEngine.ItemBase>`


Removes the *item* from the layer, as the Roto panel does: the item and its children are
deselected and removed from the Roto panel. Nothing is done if *item* is not a child of
this layer. Up to Natron 2.5, the removed item stayed in the Roto panel as an empty item
(`issue #405 <https://github.com/NatronGitHub/Natron/issues/405>`_).




