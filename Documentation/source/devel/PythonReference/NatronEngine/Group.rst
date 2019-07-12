.. module:: NatronEngine
.. _Group:

Group
*****


**Inherited by:** :doc:`Effect`, :doc:`App`, :doc:`../NatronGui/GuiApp`

Synopsis
--------

Base class for :doc:`Effect` and :doc:`App`.
See :ref:`detailed<group.details>` description below.

Functions
^^^^^^^^^

- def :meth:`getChildren<NatronEngine.Group.getChildren>` ()
- def :meth:`getNode<NatronEngine.Group.getNode>` (fullyQualifiedName)

.. _group.details:

Detailed Description
--------------------

 This is an abstract class, it is derived by 2 different classes:

    * :doc:`App` which represents an instance of Natron, or more specifically the current project.
    * :doc:`Effect` which represents a node in the node graph.

The :func:`getNode(fullyQualifiedName)<NatronEngine.Group.getNode>` can be used to retrieve
a node in the project, although all nodes already have an :ref:`auto-declared<autoVar>` variable by Natron.



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.Group.getChildren()


    :rtype: :class:`sequence`

Returns a sequence with all nodes in the group. Note that this function is not recursive
and you'd have to call **getChildren()** on all sub-groups to retrieve their children, etc...




.. method:: NatronEngine.Group.getNode(fullyQualifiedName)


    :param fullySpecifiedName: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Effect<NatronEngine.Effect>`


Retrieves a node in the group with its *fully qualified name*.
The fully qualified name of a node is the *script-name* of the node prefixed by all the
group hierarchy into which it is, e.g.:

    Blur1 # the node is a top level node


    Group1.Group2.Blur1 # the node is inside Group2 which is inside Group1

Basically you should never call this function because Natron already pre-declares a variable
for each node upon its creation.
If you were to create a new node named "Blur1" , you could the access it in the Script Editor the following way::

    app1.Blur1




