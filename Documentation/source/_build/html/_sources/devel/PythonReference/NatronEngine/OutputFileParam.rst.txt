.. module:: NatronEngine
.. _OutputFileParam:

OutputFileParam
***************

**Inherits** :doc:`StringParamBase`


Synopsis
--------

This parameter is used to specify an output file 

Functions
^^^^^^^^^

- def :meth:`openFile<NatronEngine.OutputFileParam.openFile>` ()
- def :meth:`setSequenceEnabled<NatronEngine.OutputFileParam.setSequenceEnabled>` (enabled)

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.OutputFileParam.openFile()

   When called in GUI mode, this will open a file dialog for the user. Does nothing in
   background mode.




.. method:: NatronEngine.OutputFileParam.setSequenceEnabled(enabled)


    :param enabled: :class:`bool<PySide.QtCore.bool>`

   Determines whether the file dialog opened by :func:`openFile()<NatronEngine.FileParam.openFile>`
   should have support for file sequences or not.
