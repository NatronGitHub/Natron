.. module:: NatronEngine
.. _ButtonParam:

ButtonParam
***********

**Inherits** :doc:`Param`

Synopsis
--------

A button parameter that appears in the settings panel of the node.

.. figure:: buttonParam.png

To insert code to be executed upon a user click of the button, register a function to the
onParamChanged callback on the node.

Functions
^^^^^^^^^

- def :meth:`trigger<NatronEngine.ButtonParam.trigger>` ()


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.ButtonParam.trigger()

Triggers the button action as though the user had pressed it.

