.. module:: NatronEngine
.. _ItemsTable:

ItemsTable
**********


**Inherited by:** :ref:`Roto`, :ref:`Tracker`


Synopsis
--------

A class representing a table or tree in which each row is an instance of a :ref:`ItemBase<ItemBase>`.
For instance, the RotoPaint and Tracker nodes use this class to represent their view
in the settings panel.

See detailed :ref:`description<itemsTable.details>` below.

Functions
^^^^^^^^^

- def :meth:`getItemByFullyQualifiedScriptName<NatronEngine.ItemsTable.getItemByFullyQualifiedScriptName>` (fullyQualifiedName)
- def :meth:`getTopLevelItems<NatronEngine.ItemsTable.getTopLevelItems>` ()
- def :meth:`getSelectedItems<NatronEngine.ItemsTable.getSelectedItems>` ()
- def :meth:`insertItem<NatronEngine.ItemsTable.insertItem>`(index,item,parent)
- def :meth:`removeItem<NatronEngine.ItemsTable.removeItem>`(item)
- def :meth:`getAttributeName<NatronEngine.ItemsTable.getAttributeName>`()
- def :meth:`getTableName<NatronEngine.ItemsTable.getTableName>`()
- def :meth:`isModelParentingEnabled<NatronEngine.ItemsTable.isModelParentingEnabled>`()

.. _itemsTable.details:

Detailed Description
--------------------

The ItemsTable class is a generic class representing either a table or tree view such as
the Tracker node tracks table or the RotoPaint node items tree.

Each item in the table represents a full row and is an instance of the :ref:`ItemBase<ItemBase>` class.

In the model, items may be either top-level (no parent) or child of another item.

Some models may disable the ability to have parent items, in which case all items are
considered top-level. This is the case for example of the :ref:`Tracker<Tracker>` class.
You can find out if parenting is enabled by calling :func:`isModelParentingEnabled()<NatronEngine.ItemsTable.isModelParentingEnabled>`.

:ref:`Items<ItemBase>` in the table are part of the attributes automatically defined by
Natron as explained in this :ref:`section<autovar>`.

The ItemsTable object itself is an attribute of the :ref:`effect<Effect>` holding it.
To find out the name of the Python attribute that represents the object you can call
:func:`getAttributeName()<NatronEngine.ItemsTable.getAttributeName>`.

For instance, for the RotoPaint node, the model is defined under the *roto* attribute, so
the function **getAttributeName** would return *roto*.

The model may also be accessed on the effect with the :func:`getItemsTable()<NatronEngine.Effect.getItemsTable>`
 function.

Each item in the table has a script-name that is unique with respect to its siblings:
A top-level item cannot have the same script-name as another top-level item but may have
the same script-name as a child of a top-level item.

An item is always an attribute of its parent item (if it has one) or of the ItemsTable directly.
For example on the RotoPaint node, if we have one top-level layer named Layer1 with an item
named Bezier1 inside of this layer, in Python it would be accessible as such::

    app.RotoPaint1.Layer1.Bezier1


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.ItemsTable.getItemByFullyQualifiedScriptName (fullyQualifiedScriptName)

    :param fullyQualifiedScriptName: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`ItemBase<NatronEngine.ItemBase>`

    Given an item fully qualified script-name (relative to the ItemsTable itself), returns
    the corresponding item if it exists.

    E.g::

        If we have a table as such:

        Layer1
            Layer2
                Bezier1

        The fully qualified script-name of Bezier1 is *Layer1.Layer2.Bezier1*

.. method:: NatronEngine.ItemsTable.getTopLevelItems ()

    :rtype: :class:`Sequence`

    Return a list of :ref:`items<ItemBase>` in the table that do not have a parent.

.. method:: NatronEngine.ItemsTable.getSelectedItems ()

    :rtype: :class:`Sequence`

    Return a list of selected :ref:`items<ItemBase>` in the table.


.. method:: NatronEngine.ItemsTable.insertItem (index, item, parent)

    :param index: :class:`int<PySide.QtCore.int>`
    :param item: :class:`ItemBase<NatronEngine.ItemBase>
    :param parent: :class:`ItemBase<NatronEngine.ItemBase>

    Inserts the given *item* in the table. If the model supports parenting and
    *parent* is not **None**, the *item* will be added as a child of *parent*.
    If there is a *parent*, *index* is the index at which to insert the *item* in
    the children list.
    If there is no parent, *index* is the index at which to insert the *item* in the
    table top-level items list.
    If *index* is out of range, the *item* will be added to the end of the list.


.. method:: NatronEngine.ItemsTable.removeItem (item)

    :param item: :class:`ItemBase<NatronEngine.ItemBase>

    Removes the given *item* from the model.

.. method:: NatronEngine.ItemsTable.getAttributeName ()

    :rtype: :class:`str<PySide.QtCore.QString>`

    Returns the name of the Python attribute :ref:`automatically declared<autoVar>` by Natron
    under which table items are automatically defined. For example, for the RotoPaint node,
     items are declared under the **roto** attribute.

.. method:: NatronEngine.ItemsTable.getTableName ()

    :rtype: :class:`str<PySide.QtCore.QString>`

    Returns the name of the table: this is used to identify uniquely the kind of objects
    a table may handle. Since a node may have multiple tables, each table must have a different
    name.


.. method:: NatronEngine.ItemsTable.isModelParentingEnabled()

    :rtype: :class:`bool<PySide.QtCore.bool>`

    Returns whether items may have a parent or not in this table.
