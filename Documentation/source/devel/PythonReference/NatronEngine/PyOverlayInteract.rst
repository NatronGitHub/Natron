.. module:: NatronEngine
.. _PyOverlayInteract:

PyOverlayInteract
*****************

Synopsis
--------

An overlay interact is an object used to draw and interact with on the viewer of Natron.
An overlay interact object is associated to an effect.
Usually an overlay interact is associated to a set of :ref:`Parameters<NatronEngine.PyParam>`
for which this object provides an easier interaction.
For instance, the Transform node has an overlay interact to help the user control the 
transformation directly in the viewer.

See :ref:`detailed description<PyOverlayInteract.details>` below.

Functions
^^^^^^^^^

- def :meth:`PyOverlayInteract<NatronEngine.PyOverlayInteract.PyOverlayInteract>` ()
- def :meth:`describeParameters<NatronEngine.PyOverlayInteract.describeParameters>` ()
- def :meth:`isColorPickerValid<NatronEngine.PyOverlayInteract.isColorPickerValid>` ()
- def :meth:`isColorPickerRequired<NatronEngine.PyOverlayInteract.isColorPickerRequired>` ()
- def :meth:`getHoldingEffect<NatronEngine.PyOverlayInteract.getHoldingEffect>` ()
- def :meth:`getColorPicker<NatronEngine.PyOverlayInteract.getColorPicker>` ()
- def :meth:`getPixelScale<NatronEngine.PyOverlayInteract.getPixelScale>` ()
- def :meth:`getViewportSize<NatronEngine.PyOverlayInteract.getViewportSize>` ()
- def :meth:`getSuggestedColor<NatronEngine.PyOverlayInteract.getSuggestedColor>` ()
- def :meth:`setColorPickerEnabled<NatronEngine.PyOverlayInteract.setColorPickerEnabled>` (enabled)
- def :meth:`fetchParameter<NatronEngine.PyOverlayInteract.fetchParameter>` (params, role, type, nDims, optional)
- def :meth:`fetchParameters<NatronEngine.PyOverlayInteract.fetchParameters>` (params)
- def :meth:`redraw<NatronEngine.PyOverlayInteract.redraw>` ()
- def :meth:`renderText<NatronEngine.PyOverlayInteract.renderText>` (x,y,text,r,g,b,a)
- def :meth:`draw<NatronEngine.PyOverlayInteract.draw>` (time, renderScale, view)
- def :meth:`penDown<NatronEngine.PyOverlayInteract.penDown>` (time, renderScale, view, viewportPos, penPos, pressure, timestamp, pen)
- def :meth:`penDoubleClicked<NatronEngine.PyOverlayInteract.penDoubleClicked>` (time, renderScale, view, viewportPos, penPos)
- def :meth:`penMotion<NatronEngine.PyOverlayInteract.penMotion>` (time, renderScale, view, viewportPos, penPos, pressure, timestamp)
- def :meth:`penUp<NatronEngine.PyOverlayInteract.penUp>` (time, renderScale, view, viewportPos, penPos, pressure, timestamp)
- def :meth:`keyDown<NatronEngine.PyOverlayInteract.keyDown>` (time, renderScale, view, key, modifiers)
- def :meth:`keyUp<NatronEngine.PyOverlayInteract.keyUp>` (time, renderScale, view, key, modifiers)
- def :meth:`keyRepeat<NatronEngine.PyOverlayInteract.keyRepeat>` (time, renderScale, view, key, modifiers)
- def :meth:`focusGained<NatronEngine.PyOverlayInteract.focusGained>` (time, renderScale, view)
- def :meth:`focusLost<NatronEngine.PyOverlayInteract.focusLost>` (time, renderScale, view)

.. _PyOverlayInteract.details:

Detailed Description
--------------------

This class allows to directly provide a custom overlay interact for any existing node.
To do so you need to inherit this class and provide an implementation for all the event
handlers that you need. The minimum to implement would be the :func:`draw()<NatronEngine.PyOverlayInteract.draw>`
function to provide the drawing.

The drawing itself is handled with OpenGL. You need to import the PyOpenGL package.  
Read the documentation of each function to better understand the context in which each
event handler is called on its parameters.

To actually use an interact with an effect you need to call the :func:`registerOverlay(overlay, params)<NatronEngine.Effect.registerOverlay>`
function on an :ref:`Effect<NatronEngine.Effect>`.

The interact will most likely interact with parameters of the node, these will be fetched
in the :func:`fetchParameters(params)<NatronEngine.OverlayInteract.fetchParameters>` function.
Note that this function may throw an error if a required parameter does not exist on the effect.
In case of such failure, the interact will not be added to the node.

To determine the kind of parameters needed by an interact in order to work (e.g: a Point 
interact requires at least a Double2DParam to represent the position), the 
:func:`getDescription()<NatronEngine.PyOverlayInteract.getDescription>` function must be 
implemented.
This function returns a dictionary of role names mapped against a description for a parameter.
Then the :func:`fetchParameters(params)<NatronEngine.OverlayInteract.fetchParameters>` function
takes a dictionary of role names mapped against actual parameter script-names that exist on the
effect we are calling :func:`registerOverlay(overlay, params)<NatronEngine.Effect.registerOverlay>`
on.
This description scheme allows to restrain the usage of an overlay to ensure it has a
defined behaviour.



.. _pixelScale:

**Pixel scale vs. Render scale**:
---------------------------------

Most event handlers take a *renderScale* parameter. The render scale should be multiplied
to the value of any spatial parameter, e.g: the position of a 2D point parameter.

The *getPixelScale()* function returns an additional scale that maps the coordinates of the viewport to the 
coordinates of the OpenGL orthographic projection. 

To convert a point in OpenGL ortho coordinates to viewport coordinates you divide by the 
pixel scale, and to convert from viewport coordinates to OpenGL ortho you would multiply
by the pixel scale.



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.PyOverlayInteract.PyOverlayInteract()

    Make a new overlay interact instance. This interact will only become valid when adding
    it to an Effect with the :func:`registerOverlay(overlay, params)<NatronEngine.Effect.registerOverlay>`
function.

.. method:: NatronEngine.PyOverlayInteract.describeParameters()

    :rtype: :class:`PyDict`
    
    Returns a dictionary describing all parameters requirement for this interact in order
    to function properly.
    The key of each item is a string indicating the role name of a parameter. 
    The same key is used in the :func:`fetchParameters(params)<NatronEngine.PyOverlayInteract.fetchParameters>`
    function.
    The value of each item is a tuple describing a parameter. 
    The tuple contains 3 elements:
        * A string indicating the type of the parameter that must be provided.
        This string corresponds to the value returned by the :func:`getTypeName()<NatronEngine.Param.getTypeName>`
        function of the :ref:`Param<Param>` class.
        
        * An integer indicating the number of dimensions of the paremeter, (e.g: 2 for a Double2DParam)
        
        * A boolean indicating whether this parameter is optional or not. When optional the parameter
        does not have to be provided in order for the overlay to function properly

.. method:: PyOverlayInteract.isColorPickerValid()
    
    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    Returns whether the color returned by :func:`getColorPicker()<NatronEngine.PyOverlayInteract.getColorPicker>`
    is valid or not.
    When invalid you may not assume that the value corresponds to what is currently picked
    by the cursor in the viewer.
    
    
.. method:: NatronEngine.PyOverlayInteract.isColorPickerRequired()

    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    Returns whether this interact needs the color picker information from the viewer or not.
    This is useful for example to display information related to the pixel color under the mouse.
    This is used by the ColorLookup node to help the user target a color.
    
 .. method:: NatronEngine.PyOverlayInteract.getHoldingEffect()

    :rtype: :class:`Effect<NatronEngine.Effect>`
    
    Returns the effect currently holding this interact.
    This function may return *None* if the interact was not yet registered (or removed from)
    on an effect.
    
.. method:: NatronEngine.PyOverlayInteract.getColorPicker()

    :rtype: :class:`ColorTuple<NatronEngine.ColorTuple>`
    
    Returns the color of the pixel under the mouse on the viewer.
    Note that this is only valid if the function  :func:`isColorPickerValid()<NatronEngine.PyOverlayInteract.isColorPickerValid>`
    returns *True*.
    
.. method:: NatronEngine.PyOverlayInteract.getPixelScale()

    :rtype: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    
    Returns the pixel scale of the current orthographic projection.
    See :ref:`this section<pixelScale>` for more informations
    
.. method:: NatronEngine.PyOverlayInteract.getViewportSize()

    :rtype: :class:`Int2DTuple<NatronEngine.Int2DTuple>`
    
    Returns the viewport width and height in pixel coordinates.
    
.. method:: NatronEngine.PyOverlayInteract.getSuggestedColor()

    :rtype: :class:`PyTuple`
    
    Returns a tuple with the color suggested by the user in the settings panel of the node to draw the
    overlay. This may be ignored but should be preferred to using a hard coded color.
    The first element of the tuple is a boolean indicating whether the suggested color is 
    valid or not. If invalid, you should not attempt to use it.
    The 4 next elements are the RGBA values in [0., 1.] range of the suggested color.
    
.. method:: NatronEngine.PyOverlayInteract.setColorPickerEnabled(enabled)

    :param enabled: :class:`bool<PySide.QtCore.bool>`
    
    Enable color picking: whenever the user hovers the mouse on the viewer, this interact
    will be able to retrieve the color under the mouse with the :func:`getColorPicker()<NatronEngine.PyOverlayInteract.getColorPicker>`
    function.
    
.. method:: NatronEngine.PyOverlayInteract.fetchParameter(params, role, type, nDims, optional)

    :param params: :class:`PyDict`
    :param role: :class:`QString<PySide.QtCore.QString>`
    :param type: :class:`QString<PySide.QtCore.QString>`
    :param nDims: :class:`int<PySide.QtCore.int>`
    :param optional: :class:`bool<PySide.QtCore.bool>`
    :rtype: :class:`Param<NatronEngine.Param>`
    
    Fetch a parameter for the given *role* using the dictionary *params* provided from the
    :func:`fetchParameters(params)<NatronEngine.PyOverlayInteract.fetchParameters>` function.
    *type* is the type of the parameter to fetch. This must correspond exactly to a value
    returned by the function :func:`getTypeName()<NatronEngine.Param.getTypeName>` of a sub-class of :ref:`Param<Param>`.
    *nDims* is the number of dimensions expected on the parameter (e.g: 2 for a Double2DParam)
    *optional* indicates whether this parameter is expected to be optional or not.
    If optional, this function will not fail if the parameter cannot be found.
    
    This function returns the parameter matching the given role and name if it could be found.
    
.. method:: NatronEngine.PyOverlayInteract.fetchParameters(params)

    :param params: :class:`PyDict`
    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    To be implemented to fetch parameters required to the interact. 
    The script-name of the parameters to fetch are provided by the value of each element in
    the *params* dictionary. The role fulfilled by each parameter is given by the key.
    The key must match one of those returned by the :func:`getDescription()<NatronEngine.PyOverlayInteract.getDescription>`
    function.
    
    Parameters can be fetched on the effect returned by the :func:`getHoldingEffect()<NatronEngine.OverlayInteract.getHoldingEffect>`   
    function using the helper function :func:`fetchParameter(params,role,type,nDims,optional)<NatronEngine.PyOverlayInteract.fetchParameter>`.


.. method:: NatronEngine.PyOverlayInteract.redraw()
    
    Request a redraw of the widget. This can be called in event handlers to reflect 
    a change.
    
.. method:: NatronEngine.PyOverlayInteract.renderText> (x,y,text,r,g,b,a)

    :param x: :class:`double<PySide.QtCore.double>`
    :param y: :class:`double<PySide.QtCore.double>`
    :param text: :class:`QString<PySide.QtCore.QString>`
    :param r: :class:`double<PySide.QtCore.double>`
    :param g: :class:`double<PySide.QtCore.double>`
    :param b: :class:`double<PySide.QtCore.double>`
    :param a: :class:`double<PySide.QtCore.double>`
    
    Helper function that can be used in the implementation of the :func:`draw(time,renderScale,view)<NatronEngine.PyOverlayInteract.draw>`
    function to draw text.
    The *x* and *y* coordinates are expressed in the OpenGL orthographic projection.
    
.. method:: NatronEngine.PyOverlayInteract.draw(time, renderScale, view)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
        
    Must be implemented to provide a drawing to the interact. An OpenGL context has been 
    already attached and setup with an orthographic projection so that coordinates to be passed
    to all gl* functions match those of spatial parameters.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
.. method:: NatronEngine.PyOverlayInteract.penDown(time, renderScale, view, viewportPos, penPos, pressure, timestamp, pen)
    
    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param viewportPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param penPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param pressure: :class:`double<PySide.QtCore.double>`
    :param timestamp: :class:`double<PySide.QtCore.double>`
    :param pen: :class:`PenType<NatronEngine.Natron.PenType>`
    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    Called whenever the user press a mouse button. 
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *viewportPos* is the position in **viewport** coordinates of the mouse
    
    *penPos* is the position in OpenGL orthographic projection coordinates of the mouse
    
    *pressure* is the pressure of the pen in case the event was received from a table
    
    *timestamp* is a timestamp given by the os, this is useful to implement paint brush
    interacts
    
    *pen* is the type of pen or mouse button pressed
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
        
.. method:: NatronEngine.PyOverlayInteract.penDoubleClicked(time, renderScale, view, viewportPos, penPos)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param viewportPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param penPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    Called whenever the user double cliks a mouse button. 
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *viewportPos* is the position in **viewport** coordinates of the mouse
    
    *penPos* is the position in OpenGL orthographic projection coordinates of the mouse
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.

.. method:: NatronEngine.PyOverlayInteract.penMotion(time, renderScale, view, viewportPos, penPos, pressure, timestamp)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param viewportPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param penPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param pressure: :class:`double<PySide.QtCore.double>`
    :param timestamp: :class:`double<PySide.QtCore.double>`
    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    Called whenever the user moves the mouse or pen
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *viewportPos* is the position in **viewport** coordinates of the mouse
    
    *penPos* is the position in OpenGL orthographic projection coordinates of the mouse
    
    *pressure* is the pressure of the pen in case the event was received from a table
    
    *timestamp* is a timestamp given by the os, this is useful to implement paint brush
    interacts
    
        
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
    
.. method:: NatronEngine.PyOverlayInteract.penUp(time, renderScale, view, viewportPos, penPos, pressure, timestamp)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param viewportPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param penPos: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param pressure: :class:`double<PySide.QtCore.double>`
    :param timestamp: :class:`double<PySide.QtCore.double>`
    :rtype: :class:`bool<PySide.QtCore.bool>`
    
    Called whenever the user releases a mouse button.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *viewportPos* is the position in **viewport** coordinates of the mouse
    
    *penPos* is the position in OpenGL orthographic projection coordinates of the mouse
    
    *pressure* is the pressure of the pen in case the event was received from a table
    
    *timestamp* is a timestamp given by the os, this is useful to implement paint brush
    interacts
    
        
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.

.. method:: NatronEngine.PyOverlayInteract.keyDown(time, renderScale, view, key, modifiers)
    
    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param key: :class:`Key<PySide.QtCore.Qt.Key>`
    :param modifiers: :class:`KeyboardModifiers<PySide.QtCore.KeyboardModifiers>`
    
    Called whenever the user press a key on the keyboard.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *key* is the keybind that was pressed
    
    *modifiers* is the current modifiers (Shift, Alt, Ctrl) that are held
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
    
    
.. method:: NatronEngine.PyOverlayInteract.keyUp(time, renderScale, view, key, modifiers)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param key: :class:`Key<PySide.QtCore.Qt.Key>`
    :param modifiers: :class:`KeyboardModifiers<PySide.QtCore.KeyboardModifiers>`
    
    Called whenever the user releases a key that was earlier pressed on the keyboard.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *key* is the keybind that was pressed
    
    *modifiers* is the current modifiers (Shift, Alt, Ctrl) that are held
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
    
    
.. method:: NatronEngine.PyOverlayInteract.keyRepeat(time, renderScale, view, key, modifiers)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`
    :param key: :class:`Key<PySide.QtCore.Qt.Key>`
    :param modifiers: :class:`KeyboardModifiers<PySide.QtCore.KeyboardModifiers>`
    
    Called whenever the user maintained a key pressed on the keyboard.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    *key* is the keybind that was pressed
    
    *modifiers* is the current modifiers (Shift, Alt, Ctrl) that are held
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
    
    
.. method:: NatronEngine.PyOverlayInteract.focusGained(time, renderScale, view)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`

    
    Called whenever the viewport gains focus.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
    
    
.. method:: NatronEngine.PyOverlayInteract.focusLost(time, renderScale, view)

    :param time: :class:`double<PySide.QtCore.double>`
    :param renderScale: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :param view: :class:`QString<PySide.QtCore.QString>`

    
    Called whenever the viewport gains focus.
    
    *time* is the current timeline's time. This can be passed to parameters to get a keyframe
    value at this specific time . 
    
    *view* is the current viewer's view. This can be passed to parameters to get a keyframe
    value at this specific view.
    
    *renderScale* is the current scale of the viewport. This should be multiplied to the value
    of any spatial parameters. See :ref:`this section<pixelScale>` for more information.
    
    
    This function should return *True* if it was caught and it modified something, in which
    case the event will not be propagated to other interacts. 
    If this function returns *False* the event will be propagated to other eligible interacts.
