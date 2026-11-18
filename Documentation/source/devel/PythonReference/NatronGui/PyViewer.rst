.. module:: NatronGui
.. _pyViewer:

PyViewer
************


Synopsis
-------------

A PyViewer is a wrapper around a Natron Viewer.
See :ref:`detailed<pyViewer.details>` description...

Functions
^^^^^^^^^

- def :meth:`seek<NatronGui.PyTabWidget.seek>` (frame)
- def :meth:`getCurrentFrame<NatronGui.PyTabWidget.getCurrentFrame>` ()
- def :meth:`startForward<NatronGui.PyTabWidget.startForward>` ()
- def :meth:`startBackward<NatronGui.PyTabWidget.startBackward>` ()
- def :meth:`pause<NatronGui.PyTabWidget.pause>` ()
- def :meth:`redraw<NatronGui.PyTabWidget.redraw>` ()
- def :meth:`renderCurrentFrame<NatronGui.PyTabWidget.renderCurrentFrame>` ([useCache=True])
- def :meth:`setFrameRange<NatronGui.PyTabWidget.setFrameRange>` (firstFrame,lastFrame)
- def :meth:`getFrameRange<NatronGui.PyTabWidget.getFrameRange>` ()
- def :meth:`setPlaybackMode<NatronGui.PyTabWidget.setPlaybackMode>` (mode)
- def :meth:`getPlaybackMode<NatronGui.PyTabWidget.getPlaybackMode>` ()
- def :meth:`getCompositingOperator<NatronGui.PyTabWidget.getCompositingOperator>` ()
- def :meth:`setCompositingOperator<NatronGui.PyTabWidget.setCompositingOperator>` (operator)
- def :meth:`getAInput<NatronGui.PyTabWidget.getAInput>` ()
- def :meth:`setAInput<NatronGui.PyTabWidget.setAInput>` (index)
- def :meth:`getBInput<NatronGui.PyTabWidget.getBInput>` ()
- def :meth:`setBInput<NatronGui.PyTabWidget.setBInput>` (index)
- def :meth:`setChannels<NatronGui.PyTabWidget.setChannels>` (channels)
- def :meth:`getChannels<NatronGui.PyTabWidget.getChannels>` ()
- def :meth:`setProxyModeEnabled<NatronGui.PyTabWidget.setProxyModeEnabled>` (enabled)
- def :meth:`isProxyModeEnabled<NatronGui.PyTabWidget.isProxyModeEnabled>` ()
- def :meth:`setProxyIndex<NatronGui.PyTabWidget.setProxyIndex>` (index)
- def :meth:`getProxyIndex<NatronGui.PyTabWidget.getProxyIndex>` ()
- def :meth:`setCurrentView<NatronGui.PyTabWidget.setCurrentView>` (viewIndex)
- def :meth:`getCurrentView<NatronGui.PyTabWidget.getCurrentView>` (channels)

Functions to simulate user input (for tests)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- def :meth:`penDown<NatronGui.PyViewer.penDown>` (x, y[, button=1, modifiers=0, pressure=1.0])
- def :meth:`penMotion<NatronGui.PyViewer.penMotion>` (x, y[, button=1, modifiers=0, pressure=1.0])
- def :meth:`penUp<NatronGui.PyViewer.penUp>` (x, y[, button=1, modifiers=0, pressure=1.0])
- def :meth:`penDoubleClick<NatronGui.PyViewer.penDoubleClick>` (x, y[, button=1, modifiers=0])
- def :meth:`drag<NatronGui.PyViewer.drag>` (x0, y0, x1, y1[, steps=10, button=1, modifiers=0])
- def :meth:`keyDown<NatronGui.PyViewer.keyDown>` (key[, modifiers=0])
- def :meth:`keyUp<NatronGui.PyViewer.keyUp>` (key[, modifiers=0])
- def :meth:`keyPress<NatronGui.PyViewer.keyPress>` (key[, modifiers=0])
- def :meth:`toWidgetCoordinates<NatronGui.PyViewer.toWidgetCoordinates>` (x, y)
- def :meth:`toCanonicalCoordinates<NatronGui.PyViewer.toCanonicalCoordinates>` (wx, wy)
- def :meth:`getZoom<NatronGui.PyViewer.getZoom>` ()
- def :meth:`getPan<NatronGui.PyViewer.getPan>` ()
- def :meth:`fitToFormat<NatronGui.PyViewer.fitToFormat>` ()
- def :meth:`processEvents<NatronGui.PyViewer.processEvents>` ()
- def :meth:`waitForRender<NatronGui.PyViewer.waitForRender>` ([timeoutMs=10000])

.. _pyViewer.details:

Detailed Description
---------------------------

This class is a wrapper around a Natron Viewer, exposing all functionalities available as
user interaction to the Python API.


To get a :doc:`PyViewer` , use the :func:`getViewer(scriptName)<NatronGui.GuiApp.getViewer>` function,
passing it the *script-name* of a viewer node.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronGui.PyTabWidget.seek(frame)

    :param frame: :class:`int`

Seek the timeline to a particular frame. All other viewers in the project will be synchronized
to that frame.



.. method:: NatronGui.PyTabWidget.getCurrentFrame()

    :rtype: :class:`int`

Returns the current frame on the timeline.


.. method:: NatronGui.PyTabWidget.startForward()

Starts playback, playing the video normally.


.. method:: NatronGui.PyTabWidget.startBackward()

Starts playback backward, like a rewind.


.. method:: NatronGui.PyTabWidget.pause()

Pauses the viewer if the playback is ongoing.


.. method:: NatronGui.PyTabWidget.redraw()

Redraws the OpenGL widget without actually re-rendering the internal image. This is
provided for convenience as sometimes the viewer might need refreshing for OpenGL overlays.


.. method:: NatronGui.PyTabWidget.renderCurrentFrame([useCache=True])

    :param useCache: :class:`bool`

Renders the current frame on the timeline. If *useCache* is False, the cache will not be used
and the frame will be completely re-rendered.


.. method:: NatronGui.PyTabWidget.setFrameRange(firstFrame,lastFrame)

        :param firstFrame: :class:`int`
        :param lastFrame: :class:`int`

Set the frame range on the Viewer to be \[*firstFrame* , *lastFrame*\] (included).


.. method:: NatronGui.PyTabWidget.getFrameRange()

    :rtype: :class:`Tuple`

Returns a 2-dimensional tuple of :class:`int` containing \[*firstFrame* , *lastFrame*\].


.. method:: NatronGui.PyTabWidget.setPlaybackMode(mode)

    :param mode: :class:`NatronEngine.Natron.PlaybackModeEnum`

Set the playback mode for the Viewer, it can be either **bouncing**, **looping** or
**playing once**.


.. method:: NatronGui.PyTabWidget.getPlaybackMode()

    :rtype: :class:`NatronEngine.Natron.PlaybackModeEnum`

Returns the playback mode for this Viewer.


.. method:: NatronGui.PyTabWidget.getCompositingOperator()

    :rtype: :class:`NatronEngine.Natron.ViewerCompositingOperatorEnum`

Returns the current compositing operator applied by the Viewer.


.. method:: NatronGui.PyTabWidget.setCompositingOperator(operator)

    :param operator: :class:`NatronEngine.Natron.ViewerCompositingOperatorEnum`

Set the current compositing operator applied by the Viewer.


.. method:: NatronGui.PyTabWidget.getAInput()

    :rtype: :class:`int`

Returns the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **A** choice of the Viewer.


.. method:: NatronGui.PyTabWidget.setAInput(index)

    :param index: :class:`int`

Set the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **A** choice of the Viewer.



.. method:: NatronGui.PyTabWidget.getBInput()

    :rtype: :class:`int`

Returns the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **B** choice of the Viewer.


.. method:: NatronGui.PyTabWidget.setBInput(index)

    :param index: :class:`int`

Set the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **B** choice of the Viewer.


.. method:: NatronGui.PyTabWidget.setChannels(channels)

    :param channels: :class:`NatronEngine.Natron.DisplayChannelsEnum`

Set the *channels* to be displayed on the Viewer.


.. method:: NatronGui.PyTabWidget.getChannels()

    :rtype: :class:`NatronEngine.Natron.DisplayChannelsEnum`

Returns the current *channels*  displayed on the Viewer.


.. method:: NatronGui.PyTabWidget.setProxyModeEnabled(enabled)

    :param enabled: :class:`bool`

Set the proxy mode *enabled*.

.. method:: NatronGui.PyTabWidget.isProxyModeEnabled(enabled)

    :rtype: :class:`bool`

Returns whether the proxy mode is *enabled*.


.. method:: NatronGui.PyTabWidget.setProxyIndex(index)

    :param index: :class:`int`

Set the *index* of the proxy to use. This is the index in the combobox on the graphical
user interface, e.g.  *index = 0* will be *2*

.. method:: NatronGui.PyTabWidget.getProxyIndex()

    :rtype: :class:`int`

Returns the *index* of the proxy in use. This is the index in the combobox on the graphical
user interface, e.g.  *index = 0* will be *2*


.. method:: NatronGui.PyTabWidget.setCurrentView(viewIndex)

    :param viewIndex: :class:`int`

Set the view to display the given *viewIndex*. This is the index in the multi-view combobox
visible when the number of views in the project settings has been set to a value greater than 1.

.. method:: NatronGui.PyTabWidget.getCurrentView()

    :param viewIndex: :class:`int`

Returns the currently  displayed view index. This is the index in the multi-view combobox
visible when the number of views in the project settings has been set to a value greater than 1.


Simulating user input
^^^^^^^^^^^^^^^^^^^^^

These functions send real Qt mouse, tablet and key events to the OpenGL widget of the viewer,
so that a script or a test can drive the viewer and the overlays of the nodes (OpenFX
interacts, Transform, Roto, Tracker...) as a user would, and check the result, e.g. the values
of the parameters. The events go through the viewer's own handling (pan, zoom, picking) and the
overlays, with the same keyboard modifiers.
They work when the window is not focused or not on top, and they never move the mouse cursor.
See :ref:`guiTesting` for a complete example.

The positions are in **canonical coordinates**: the coordinates the overlays receive, with the
pixel aspect ratio of the image applied and the y axis pointing up (the origin is the
bottom-left corner of the project format).
Use :meth:`fitToFormat<NatronGui.PyViewer.fitToFormat>` and
:meth:`waitForRender<NatronGui.PyViewer.waitForRender>` first, to get a known view.

*button* is a :class:`PySide.QtCore.Qt.MouseButton` value: 1 for the left button, 2 for the
right button, 4 for the middle button. *modifiers* is a combination of
:class:`PySide.QtCore.Qt.KeyboardModifier` values (e.g. ``QtCore.Qt.ShiftModifier``), and *key*
is a :class:`PySide.QtCore.Qt.Key` (e.g. ``QtCore.Qt.Key_F``), as in
:meth:`addMenuCommand<NatronGui.PyGuiApplication.addMenuCommand>`.

The pen and key functions return True if the event was accepted by the viewer or an overlay.
Most events are accepted, so check the result of the action instead.

The overlay of a node is only shown, and only receives the events, while its settings panel is
open. :meth:`App.createNode<NatronEngine.App.createNode>` does not open it: select the node
with :meth:`selectNode<NatronGui.GuiApp.selectNode>` and send the *Return* key to the node
graph, as in the example of :ref:`guiTesting`.

.. method:: NatronGui.PyViewer.penDown(x, y[, button=1, modifiers=0, pressure=1.0])

    :param x: :class:`float`
    :param y: :class:`float`
    :param button: :class:`int`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :param pressure: :class:`float`
    :rtype: :class:`bool`

Presses *button* at the canonical position (*x*, *y*). A *pressure* other than 1 also sends
tablet events, as a pen tablet would.

.. method:: NatronGui.PyViewer.penMotion(x, y[, button=1, modifiers=0, pressure=1.0])

    :param x: :class:`float`
    :param y: :class:`float`
    :param button: :class:`int`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :param pressure: :class:`float`
    :rtype: :class:`bool`

Moves the pointer to the canonical position (*x*, *y*) with *button* held (0 for a motion
without any button pressed).

.. method:: NatronGui.PyViewer.penUp(x, y[, button=1, modifiers=0, pressure=1.0])

    :param x: :class:`float`
    :param y: :class:`float`
    :param button: :class:`int`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :param pressure: :class:`float`
    :rtype: :class:`bool`

Releases *button* at the canonical position (*x*, *y*).

.. method:: NatronGui.PyViewer.penDoubleClick(x, y[, button=1, modifiers=0])

    :param x: :class:`float`
    :param y: :class:`float`
    :param button: :class:`int`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :rtype: :class:`bool`

Double-clicks at the canonical position (*x*, *y*): sends the events Qt sends for a
double-click (press, release, double-click, release). Returns whether the double-click event
was accepted.

.. method:: NatronGui.PyViewer.drag(x0, y0, x1, y1[, steps=10, button=1, modifiers=0])

    :param x0: :class:`float`
    :param y0: :class:`float`
    :param x1: :class:`float`
    :param y1: :class:`float`
    :param steps: :class:`int`
    :param button: :class:`int`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :rtype: :class:`bool`

Presses *button* at (*x0*, *y0*), moves the pointer to (*x1*, *y1*) in *steps* motions, and
releases the button there. Returns False if the press was not accepted.

.. method:: NatronGui.PyViewer.keyDown(key[, modifiers=0])

    :param key: :class:`PySide.QtCore.Qt.Key`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :rtype: :class:`bool`

Sends a key press to the viewer. As for a key typed by the user, the overlays get it first,
then the viewer shortcuts (e.g. *F* fits the image, *+* zooms in).

.. method:: NatronGui.PyViewer.keyUp(key[, modifiers=0])

    :param key: :class:`PySide.QtCore.Qt.Key`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :rtype: :class:`bool`

Sends a key release to the viewer.

.. method:: NatronGui.PyViewer.keyPress(key[, modifiers=0])

    :param key: :class:`PySide.QtCore.Qt.Key`
    :param modifiers: :class:`PySide.QtCore.Qt.KeyboardModifiers`
    :rtype: :class:`bool`

Same as :meth:`keyDown<NatronGui.PyViewer.keyDown>` followed by
:meth:`keyUp<NatronGui.PyViewer.keyUp>`. Returns whether the key press was accepted.

.. method:: NatronGui.PyViewer.toWidgetCoordinates(x, y)

    :param x: :class:`float`
    :param y: :class:`float`
    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`

Converts the canonical position (*x*, *y*) to the coordinates of the viewer's OpenGL widget,
in logical pixels (as in Qt events), with the y axis pointing down.

.. method:: NatronGui.PyViewer.toCanonicalCoordinates(wx, wy)

    :param wx: :class:`float`
    :param wy: :class:`float`
    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`

The inverse of :meth:`toWidgetCoordinates<NatronGui.PyViewer.toWidgetCoordinates>`.

.. method:: NatronGui.PyViewer.getZoom()

    :rtype: :class:`float`

Returns the zoom factor of the viewer: the number of widget pixels per canonical unit.

.. method:: NatronGui.PyViewer.getPan()

    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`

Returns the canonical coordinates of the bottom-left corner of the viewer.

.. method:: NatronGui.PyViewer.fitToFormat()

Fits the project format in the viewer, as the *F* key does.

.. method:: NatronGui.PyViewer.processEvents()

Processes the pending GUI events, e.g. to let the viewer redraw its overlays.

.. method:: NatronGui.PyViewer.waitForRender([timeoutMs=10000])

    :param timeoutMs: :class:`int`
    :rtype: :class:`bool`

Processes the GUI events until the viewer has no render in progress, so that the image and the
overlays are up to date, e.g. before checking the values of the parameters or calling
:meth:`saveScreenshot<NatronGui.GuiApp.saveScreenshot>`. Returns False if the viewer was still
rendering after *timeoutMs* milliseconds.
