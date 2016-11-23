.. module:: NatronEngine
.. _PathParam:

PathParam
*********

**Inherits** :doc:`StringParamBase`


Synopsis
--------

A path param is used to indicate the path to a directory.
See :ref:`details<dirdetails>`...

Functions
^^^^^^^^^

*    def :meth:`setAsMultiPathTable<NatronEngine.PathParam.setAsMultiPathTable>` ()
*    def :meth:`isMultiPathTable<NatronEngine.PathParam.isMultiPathTable>` ()
*    def :meth:`getTable<NatronEngine.PathParam.getTable>` ()


.. _dirdetails:

Detailed Description
--------------------

By default the user can select a single directory as path, unless 
:func:`setAsMultiPathTable()<NatronEngine.PathParam.setAsMultiPathTable>` is called in which
case a table is presented to the user to specify multiple directories like this:

.. figure:: multiPathParam.png
	:width: 400px
	:align: center
	
When using multiple paths, internally they are separated by a *;* and the following characters
are escaped as per the XML specification:

	* *<* becomes &lt;
	* *>* becomes &gt;
	* *&* becomes &amp;
	* *"* becomes &quot;
	* *'* becomes &apos;
	
Some more characters are escaped, you can see the full function in the source code of Natron
`here <https://github.com/MrKepzie/Natron/blob/master/Engine/ProjectPrivate.cpp>`_



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.PathParam.setAsMultiPathTable()


When called, the parameter will be able to store multiple paths.

.. method:: NatronEngine.PathParam.isMultiPathTable()


	:rtype: :class:`bool<PySide.QtCore.bool>`
	
	
Returns whether this path parameter is set as a table containing multiple paths or not.



.. method:: NatronEngine.PathParam.getTable()

	:rtype: :class:`PySequence`
	

If this parameter is a multi-path table, returns a sequence of the rows of the table.
Each row is a sequence of strings.
This can only be called if the function :func:`isMultiPathTable<NatronEngine.PathParam.isMultiPathTable>`
returns True.







