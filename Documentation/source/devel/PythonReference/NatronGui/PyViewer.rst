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
