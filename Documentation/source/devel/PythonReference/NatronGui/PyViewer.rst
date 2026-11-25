.. currentmodule:: NatronGui
.. _pyViewer:

PyViewer
************


Synopsis
-------------

A PyViewer is a wrapper around a Natron Viewer.
See :ref:`detailed<pyViewer.details>` description...

Functions
^^^^^^^^^

- def :meth:`seek<NatronGui.PyViewer.seek>` (frame)
- def :meth:`getCurrentFrame<NatronGui.PyViewer.getCurrentFrame>` ()
- def :meth:`startForward<NatronGui.PyViewer.startForward>` ()
- def :meth:`startBackward<NatronGui.PyViewer.startBackward>` ()
- def :meth:`pause<NatronGui.PyViewer.pause>` ()
- def :meth:`redraw<NatronGui.PyViewer.redraw>` ()
- def :meth:`renderCurrentFrame<NatronGui.PyViewer.renderCurrentFrame>` ([useCache=True])
- def :meth:`setFrameRange<NatronGui.PyViewer.setFrameRange>` (firstFrame,lastFrame)
- def :meth:`getFrameRange<NatronGui.PyViewer.getFrameRange>` ()
- def :meth:`setPlaybackMode<NatronGui.PyViewer.setPlaybackMode>` (mode)
- def :meth:`getPlaybackMode<NatronGui.PyViewer.getPlaybackMode>` ()
- def :meth:`getCompositingOperator<NatronGui.PyViewer.getCompositingOperator>` ()
- def :meth:`setCompositingOperator<NatronGui.PyViewer.setCompositingOperator>` (operator)
- def :meth:`getAInput<NatronGui.PyViewer.getAInput>` ()
- def :meth:`setAInput<NatronGui.PyViewer.setAInput>` (index)
- def :meth:`getBInput<NatronGui.PyViewer.getBInput>` ()
- def :meth:`setBInput<NatronGui.PyViewer.setBInput>` (index)
- def :meth:`setChannels<NatronGui.PyViewer.setChannels>` (channels)
- def :meth:`getChannels<NatronGui.PyViewer.getChannels>` ()
- def :meth:`setProxyModeEnabled<NatronGui.PyViewer.setProxyModeEnabled>` (enabled)
- def :meth:`isProxyModeEnabled<NatronGui.PyViewer.isProxyModeEnabled>` ()
- def :meth:`setProxyIndex<NatronGui.PyViewer.setProxyIndex>` (index)
- def :meth:`getProxyIndex<NatronGui.PyViewer.getProxyIndex>` ()
- def :meth:`setCurrentView<NatronGui.PyViewer.setCurrentView>` (viewIndex)
- def :meth:`getCurrentView<NatronGui.PyViewer.getCurrentView>` ()
- def :meth:`fit<NatronGui.PyViewer.fit>` ()
- def :meth:`getZoom<NatronGui.PyViewer.getZoom>` ()
- def :meth:`setZoom<NatronGui.PyViewer.setZoom>` (zoom)
- def :meth:`getCenter<NatronGui.PyViewer.getCenter>` ()
- def :meth:`setCenter<NatronGui.PyViewer.setCenter>` (x,y)
- def :meth:`canvasToWidget<NatronGui.PyViewer.canvasToWidget>` (x,y)
- def :meth:`widgetToCanvas<NatronGui.PyViewer.widgetToCanvas>` (x,y)
- def :meth:`getPixel<NatronGui.PyViewer.getPixel>` (x,y[,input=0])
- def :meth:`getImage<NatronGui.PyViewer.getImage>` ([input=0[, region]])

.. _pyViewer.details:

Detailed Description
---------------------------

This class is a wrapper around a Natron Viewer, exposing all functionalities available as
user interaction to the Python API.


To get a :doc:`PyViewer` , use the :func:`getViewer(scriptName)<NatronGui.GuiApp.getViewer>` function,
passing it the *script-name* of a viewer node.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: PyViewer.seek(frame)

    :param frame: :class:`int`

Seek the timeline to a particular frame. All other viewers in the project will be synchronized
to that frame.



.. method:: PyViewer.getCurrentFrame()

    :rtype: :class:`int`

Returns the current frame on the timeline.


.. method:: PyViewer.startForward()

Starts playback, playing the video normally.


.. method:: PyViewer.startBackward()

Starts playback backward, like a rewind.


.. method:: PyViewer.pause()

Pauses the viewer if the playback is ongoing.


.. method:: PyViewer.redraw()

Redraws the OpenGL widget without actually re-rendering the internal image. This is
provided for convenience as sometimes the viewer might need refreshing for OpenGL overlays.


.. method:: PyViewer.renderCurrentFrame([useCache=True])

    :param useCache: :class:`bool`

Renders the current frame on the timeline. If *useCache* is False, the cache will not be used
and the frame will be completely re-rendered.


.. method:: PyViewer.setFrameRange(firstFrame,lastFrame)

        :param firstFrame: :class:`int`
        :param lastFrame: :class:`int`

Set the frame range on the Viewer to be \[*firstFrame* , *lastFrame*\] (included).


.. method:: PyViewer.getFrameRange()

    :rtype: :class:`Tuple`

Returns a 2-dimensional tuple of :class:`int` containing \[*firstFrame* , *lastFrame*\].


.. method:: PyViewer.setPlaybackMode(mode)

    :param mode: :class:`NatronEngine.Natron.PlaybackModeEnum`

Set the playback mode for the Viewer, it can be either **bouncing**, **looping** or
**playing once**.


.. method:: PyViewer.getPlaybackMode()

    :rtype: :class:`NatronEngine.Natron.PlaybackModeEnum`

Returns the playback mode for this Viewer.


.. method:: PyViewer.getCompositingOperator()

    :rtype: :class:`NatronEngine.Natron.ViewerCompositingOperatorEnum`

Returns the current compositing operator applied by the Viewer.


.. method:: PyViewer.setCompositingOperator(operator)

    :param operator: :class:`NatronEngine.Natron.ViewerCompositingOperatorEnum`

Set the current compositing operator applied by the Viewer.


.. method:: PyViewer.getAInput()

    :rtype: :class:`int`

Returns the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **A** choice of the Viewer.


.. method:: PyViewer.setAInput(index)

    :param index: :class:`int`

Set the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **A** choice of the Viewer.



.. method:: PyViewer.getBInput()

    :rtype: :class:`int`

Returns the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **B** choice of the Viewer.


.. method:: PyViewer.setBInput(index)

    :param index: :class:`int`

Set the **index** of the input (the same index used by :func:`getInput(index)<NatronEngine.Effect.getInput>`) used by
the **B** choice of the Viewer.


.. method:: PyViewer.setChannels(channels)

    :param channels: :class:`NatronEngine.Natron.DisplayChannelsEnum`

Set the *channels* to be displayed on the Viewer.


.. method:: PyViewer.getChannels()

    :rtype: :class:`NatronEngine.Natron.DisplayChannelsEnum`

Returns the current *channels*  displayed on the Viewer.


.. method:: PyViewer.setProxyModeEnabled(enabled)

    :param enabled: :class:`bool`

Set the proxy mode *enabled*.

.. method:: PyViewer.isProxyModeEnabled()

    :rtype: :class:`bool`

Returns whether the proxy mode is *enabled*.


.. method:: PyViewer.setProxyIndex(index)

    :param index: :class:`int`

Set the *index* of the proxy to use. This is the index in the combobox on the graphical
user interface, e.g.  *index = 0* will be *2*

.. method:: PyViewer.getProxyIndex()

    :rtype: :class:`int`

Returns the *index* of the proxy in use. This is the index in the combobox on the graphical
user interface, e.g.  *index = 0* will be *2*


.. method:: PyViewer.setCurrentView(viewIndex)

    :param viewIndex: :class:`int`

Set the view to display the given *viewIndex*. This is the index in the multi-view combobox
visible when the number of views in the project settings has been set to a value greater than 1.

.. method:: PyViewer.getCurrentView()

    :rtype: :class:`int`

Returns the currently  displayed view index. This is the index in the multi-view combobox
visible when the number of views in the project settings has been set to a value greater than 1.

.. method:: PyViewer.fit()

    Fits the image to the viewer, as the *F* key does.

.. method:: PyViewer.getZoom()

    :rtype: :class:`float`

    Returns the zoom factor of the viewer: 1 shows one image pixel per screen pixel (100%).

.. method:: PyViewer.setZoom(zoom)

    :param zoom: :class:`float`

    Sets the zoom factor of the viewer (see :meth:`getZoom()<NatronGui.PyViewer.getZoom>`), keeping the
    point at the centre of the viewport fixed, as the zoom menu of the viewer does.

.. method:: PyViewer.getCenter()

    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`

    Returns the canvas coordinates of the centre of the viewport. The canvas coordinates are the
    coordinates of the image and of the overlays: the origin is the bottom-left corner of the project
    format, *y* points up, and the pixel aspect ratio is included.

.. method:: PyViewer.setCenter(x, y)

    :param x: :class:`float`
    :param y: :class:`float`

    Pans the viewer so that the canvas point (*x*, *y*) is at the centre of the viewport.

.. method:: PyViewer.canvasToWidget(x, y)

    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`

    Converts the canvas coordinates (*x*, *y*) to the coordinates of the viewer widget, in Qt logical
    pixels with the origin at the top-left corner, e.g. to place a PySide widget over the viewer.

.. method:: PyViewer.widgetToCanvas(x, y)

    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`

    Converts the coordinates of the viewer widget to canvas coordinates
    (the inverse of :meth:`canvasToWidget()<NatronGui.PyViewer.canvasToWidget>`).

.. method:: PyViewer.getPixel(x, y[, input=0])

    :param input: :class:`int`
    :rtype: :class:`ColorTuple<NatronEngine.ColorTuple>`

    Returns the linear RGBA value of the last rendered image at the canvas point (*x*, *y*) for the input
    A (0) or B (1), before the display LUT: the value of the colour picker. Call
    :meth:`App.waitForRenders()<NatronEngine.App.waitForRenders>` first: during a render, it is the value
    of the previous image. Raises :class:`ValueError` if there is no image at that point, and
    :class:`RuntimeError` when not called from the main thread.

.. method:: PyViewer.getImage([input=0[, region]])

    :param input: :class:`int`
    :param region: :class:`RectD<NatronEngine.RectD>`
    :rtype: :class:`tuple`

    Returns the linear RGBA image of the last render for the input A (0) or B (1), before the display
    LUT, within *region* (canvas coordinates; the whole image if omitted or *None*), as a tuple
    ``(bounds, pixels)``. *bounds* is a :class:`RectD<NatronEngine.RectD>` with the area of the returned
    pixels in canvas coordinates. *pixels* is a :class:`bytes` object holding four 32-bit floats (R, G, B,
    A) per pixel, row by row from the bottom row: read it with ``array.array('f', pixels)`` or
    ``numpy.frombuffer(pixels, numpy.float32)``. The image is at the mipmap level that the viewer
    rendered for its zoom (1 image pixel per 2\ :sup:`level` canvas units; the bounds tell the size), or
    the closest level rendered. Call :meth:`App.waitForRenders()<NatronEngine.App.waitForRenders>` first:
    during a render, it is the previous image. Raises :class:`ValueError` if the viewer shows no image
    (in that region), and :class:`RuntimeError` when not called from the main thread.
