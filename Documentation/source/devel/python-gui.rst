.. _python-gui:

Extending the user interface
============================

This section covers the functions of the NatronGui module, which only exist in GUI mode:
dialogs, menu commands, Python panels and the viewer. Put this code in
:ref:`initGui.py<startupScripts>`, or guard it with ``if not natron.isBackground():``.

.. toctree::
    :maxdepth: 1

    modalDialogs.rst
    userCommands.rst
    pysidePanelExample.rst
    viewersInteraction.rst
