.. _viewersInteraction:

Controlling the viewer
======================

<<<<<<< HEAD
The viewer in Natron Python A.P.I can be controlled much like any other nodes::
=======
Natron exposes all functionalities available to the user in the Python API via the
:ref:`PyViewer<pyViewer>` class.
>>>>>>> RB-2.3

    app1.Viewer1

<<<<<<< HEAD
Parameters in the Viewer interface inherit the :ref:`Param<Param>` class and can be
retrieved by their :ref:`script-name<autovar>` ::

    app1.Viewer1.gain.set(2)

You can then control the player, the displayed channels, the current view, the current
compositing operator, which are the input A and B,  the frame-range, the proxy level and
various other stuff with the parameters.

In GUI mode only, you can access the last viewer that was interacted with by the user::

    viewerNode = app1.getActiveViewer()

You can redraw a viewer or re-render the viewer texture by calling the following functions::


    # Refresh the viewer texture. This causes a re-evaluation of the node-graph.
    # If the second boolean parameter is set to True, the render will not attempt
    # to retrieve a texture from the cache if there is any.
    app1.refreshViewer(viewerNode, False)
=======
    app.pane2.Viewer1

or use the following function  :func:`getViewer(scriptName)<NatronGui.GuiApp.getViewer>` ,
passing it the *script-name* of a viewer node.

You can then control the player, the displayed channels, the current view, the current
compositing operator, which are the input A and B,  the frame-range, the proxy level and
various other stuff.
>>>>>>> RB-2.3

    # Just redraws the OpenGL viewer. The internal texture displayed will not be
    # re-evaluated.
    app1.redrawViewer(viewerNode)
