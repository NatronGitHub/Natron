.. module:: NatronEngine
.. _FileParam:

FileParam
*********

**Inherits** :doc:`StringParamBase`

Synopsis
--------

This parameter is used to specify an input file (i.e: a file that already exist). 

Functions
^^^^^^^^^

- def :meth:`openFile<NatronEngine.FileParam.openFile>` ()
- def :meth:`reloadFile<NatronEngine.FileParam.reloadFile>` ()
- def :meth:`setSequenceEnabled<NatronEngine.FileParam.setSequenceEnabled>` (enabled)

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.FileParam.openFile()


When called in GUI mode, this will open a file dialog for the user. Does nothing in 
background mode.


.. method:: NatronEngine.FileParam.reloadFile()


Force a refresh of the data read from the file. Any cached data associated to the file will be 
discarded. 




.. method:: NatronEngine.FileParam.setSequenceEnabled(enabled)


    :param enabled: :class:`bool<PySide.QtCore.bool>`

Determines whether the file dialog opened by :func:`openFile()<NatronEngine.FileParam.openFile>`
should have support for file sequences or not.





