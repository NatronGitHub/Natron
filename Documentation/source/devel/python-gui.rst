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

.. _widgetNames:

Widget names
------------

The main widgets of Natron have a stable ``objectName()``, so that PySide code, Qt style sheets,
accessibility tools and GUI tests can find them (e.g. with ``QWidget.findChild()``):

- the node graph of the project: ``nodeGraph``;
- the settings panel of a node: the fully qualified name of the node, e.g. ``Transform1`` or
  ``Group1.Blur1``;
- the widgets of a parameter in a settings panel: the script name of the parameter, e.g.
  ``translate``. Every widget of the parameter also has the Qt property ``paramName``, which is
  the script name of the parameter, including the widgets that have another name (e.g. the
  ``dim-0`` field of a dimension). A row of parameters is named after its first parameter;
- a viewer tab and its OpenGL widget: the script name of the tab, e.g. ``Viewer1`` (as for
  :meth:`GuiApp.getTabWidget()<NatronGui.GuiApp.getTabWidget>`);
- the node creation dialog (the *Tab* key in the node graph): ``nodeCreationDialog``.
