.. _viewersInteraction:

Controlling the viewer
======================

Natron exposes all functionalities available to the user in the Python API via the
:ref:`PyViewer<pyViewer>` class.

To retrieve a :ref:`PyViewer<pyViewer>`, use the :ref:`auto-declared<autoVar>` variable::

    app.pane2.Viewer1

or use the following function  :func:`getViewer(scriptName)<NatronGui.GuiApp.getViewer>` ,
passing it the *script-name* of a viewer node.

You can then control the player, the displayed channels, the current view, the current
compositing operator, which are the input A and B,  the frame-range, the proxy level and
various other stuff.

