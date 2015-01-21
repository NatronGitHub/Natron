.. module:: NatronEngine
.. _Group:

Group
*****


**Inherited by:** :ref:`Effect`, :ref:`App`, :ref:`GuiApp`

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`getChildren<NatronEngine.Group.getChildren>` ()
*    def :meth:`getNode<NatronEngine.Group.getNode>` (fullySpecifiedName)


Detailed Description
--------------------


    
    This is an abstract class, it is derived by 2 different classes:
    :doc:`App` which represents an instance of Natron, or more specifically the current project.
    :doc:`Effect` which represents an a node in the nodegraph.
    


.. class:: Group()



.. method:: NatronEngine.Group.getChildren()


    :rtype: 






.. method:: NatronEngine.Group.getNode(fullySpecifiedName)


    :param fullySpecifiedName: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Effect`







