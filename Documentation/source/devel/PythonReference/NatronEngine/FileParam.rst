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


*    def :meth:`openFile<NatronEngine.FileParam.openFile>` ()
*    def :meth:`reloadFile<NatronEngine.FileParam.reloadFile>` ()
*    def :meth:`setSequenceEnabled<NatronEngine.FileParam.setSequenceEnabled>` (enabled)
*    def :meth:`setDialogType<NatronEngine.FileParam.setDialogType>` (existingFiles, useSequences, fileTypes)


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

This method is deprecated. Use :func:`setDialogType<NatronEngine.FileParam.setDialogType>` instead.
Determines whether the file dialog opened by :func:`openFile()<NatronEngine.FileParam.openFile>`
should have support for file sequences or not.

.. method:: NatronEngine.FileParam.setDialogType (existingFiles, useSequences, fileTypes)

    :param existingFiles: :class:`bool<PySide.QtCore.bool>`
    :param useSequences: :class:`bool<PySide.QtCore.bool>`
    :param fileTypes: :class:`Sequence`

    Set the kind of file selector this parameter should be.
    If *existingFiles* is set to **True** then the dialog will only be able to select existing
    files, otherwise the user will be able to specify non-existing files. The later case
    is useful when asking the user for a location where to save a file.

    If *useSequences* is **True** then the file dialog will be able to gather files by sequences.
    This is mostly useful when you need to retrieve images from the user.

    *fileTypes* indicates a list of file types accepted by the dialog.
