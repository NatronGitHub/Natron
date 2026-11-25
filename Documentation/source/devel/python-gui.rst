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

- the node graph of the project: ``nodeGraph`` (its tab script name); the node graph of a group: the
  fully qualified name of the group, e.g. ``Group1``;
- the settings panel of a node: the fully qualified name of the node, e.g. ``Transform1`` or
  ``Group1.Blur1`` (updated when the node or its group is renamed);
- the widgets of a parameter in a settings panel: the first one that has no other name (e.g. not the
  ``dim-0`` field of a dimension) gets the script name of the parameter, e.g. ``translate``, so that the
  name is unique in the panel; every widget of the parameter has the Qt property ``paramName``, the
  script name of the parameter. A row of parameters is named ``<param>Row``, after its first parameter;
- a viewer tab and its OpenGL widget: the script name of the tab, e.g. ``Viewer1`` (as for
  :meth:`GuiApp.getTabWidget()<NatronGui.GuiApp.getTabWidget>`); the timeline of a viewer: ``TimeLine``;
- the properties bin: ``PropertiesBin``;
- the node creation dialog (the *Tab* key in the node graph): ``nodeCreationDialog``, and its line edit
  ``NodeCreationDialogLineEdit``.
