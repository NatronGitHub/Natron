.. module:: NatronEngine
.. _Effect:

Effect
******

**Inherits**: :doc:`Group` , :doc:`UserParamHolder`

Synopsis
--------

This object represents a single node in Natron, that is: an instance of a plug-in.
See :ref:`Effectdetails`

Functions
^^^^^^^^^

- def :meth:`addUserPlane<NatronEngine.Effect.addUserPlane>` (planeName,channels)
- def :meth:`endChanges<NatronEngine.Effect.endChanges>` ()
- def :meth:`beginChanges<NatronEngine.Effect.beginChanges>` ()
- def :meth:`beginParametersUndoCommand<NatronEngine.Effect.beginParametersUndoCommand>` (commandName)
- def :meth:`endParametersUndoCommand<NatronEngine.Effect.endParametersUndoCommand>` ()
- def :meth:`canConnectInput<NatronEngine.Effect.canConnectInput>` (inputNumber, node)
- def :meth:`connectInput<NatronEngine.Effect.connectInput>` (inputNumber, input)
- def :meth:`insertParamInViewerUI<NatronEngine.Effect.insertParamInViewerUI>` (parameter[, index=-1])
- def :meth:`removeParamFromViewerUI<NatronEngine.Effect.removeParamFromViewerUI>` (parameter)
- def :meth:`clearViewerUIParameters<NatronEngine.Effect.clearViewerUIParameters>` ()
- def :meth:`destroy<NatronEngine.Effect.destroy>` ([autoReconnect=true])
- def :meth:`disconnectInput<NatronEngine.Effect.disconnectInput>` (inputNumber)
- def :meth:`getAvailableLayers<NatronEngine.Effect.getAvailableLayers>` (inputNumber)
- def :meth:`getBitDepth<NatronEngine.Effect.getBitDepth>` ()
- def :meth:`getColor<NatronEngine.Effect.getColor>` ()
- def :meth:`getContainerGroup<NatronEngine.Effect.getContainerGroup>` ()
- def :meth:`getCurrentTime<NatronEngine.Effect.getCurrentTime>` ()
- def :meth:`getOutputFormat<NatronEngine.Effect.getOutputFormat>` ()
- def :meth:`getFrameRate<NatronEngine.Effect.getFrameRate>` ()
- def :meth:`getInput<NatronEngine.Effect.getInput>` (inputNumber)
- def :meth:`getInput<NatronEngine.Effect.getInput>` (inputName)
- def :meth:`getLabel<NatronEngine.Effect.getLabel>` ()
- def :meth:`getInputLabel<NatronEngine.Effect.getInputLabel>` (inputNumber)
- def :meth:`getMaxInputCount<NatronEngine.Effect.getMaxInputCount>` ()
- def :meth:`getParam<NatronEngine.Effect.getParam>` (name)
- def :meth:`getParams<NatronEngine.Effect.getParams>` ()
- def :meth:`getPluginID<NatronEngine.Effect.getPluginID>` ()
- def :meth:`getPosition<NatronEngine.Effect.getPosition>` ()
- def :meth:`getPremult<NatronEngine.Effect.getPremult>` ()
- def :meth:`getPixelAspectRatio<NatronEngine.Effect.getPixelAspectRatio>` ()
- def :meth:`getRegionOfDefinition<NatronEngine.Effect.getRegionOfDefinition>` (time,view)
- def :meth:`getItemsTable<NatronEngine.Effect.getItemsTable>` (tableName)
- def :meth:`getAllItemsTable<NatronEngine.Effect.getAllItemsTable>` ()
- def :meth:`getScriptName<NatronEngine.Effect.getScriptName>` ()
- def :meth:`getSize<NatronEngine.Effect.getSize>` ()
- def :meth:`getUserPageParam<NatronEngine.Effect.getUserPageParam>` ()
- def :meth:`isNodeActivated<NatronEngine.Effect.isNodeActivated>` ()
- def :meth:`isUserSelected<NatronEngine.Effect.isUserSelected>` ()
- def :meth:`isReaderNode<NatronEngine.Effect.isReaderNode>` ()
- def :meth:`isWriterNode<NatronEngine.Effect.isWriterNode>` ()
- def :meth:`setColor<NatronEngine.Effect.setColor>` (r, g, b)
- def :meth:`setLabel<NatronEngine.Effect.setLabel>` (name)
- def :meth:`setPosition<NatronEngine.Effect.setPosition>` (x, y)
- def :meth:`setScriptName<NatronEngine.Effect.setScriptName>` (scriptName)
- def :meth:`setSize<NatronEngine.Effect.setSize>` (w, h)
- def :meth:`setSubGraphEditable<NatronEngine.Effect.setSubGraphEditable>` (editable)
- def :meth:`setPagesOrder<NatronEngine.Effect.setPagesOrder>` (pages)
- def :meth:`registerOverlay<NatronEngine.Effect.registerOverlay>` (overlay, params)
- def :meth:`removeOverlay<NatronEngine.Effect.removeOverlay>` (overlay)

.. _Effectdetails:

Detailed Description
--------------------

The Effect object can be used to operate with a single node in Natron.
To create a new Effect, use the :func:`app.createNode(pluginID)<NatronEngine.App.createNode>` function.

Natron automatically declares a variable to Python when a new Effect is created.
This variable will have a script-name determined by Natron as explained in the
:ref:`autovar` section.

Once an Effect is instantiated, it declares all its :doc:`Param` and inputs.
See how to :ref:`manage <userParams>` user parameters below

To get a specific :doc:`Param` by script-name, call the
:func:`getParam(name) <NatronEngine.Effect.getParam>` function

Input effects are mapped against a zero-based index. To retrieve an input Effect
given an index, you can use the :func:`getInput(inputNumber) <NatronEngine.Effect.getInput>`
function.

To manage inputs, you can connect them and disconnect them with respect to their input
index with the :func:`connectInput(inputNumber,input)<NatronEngine.Effect.connectInput>` and
then :func:`disconnectInput(inputNumber)<NatronEngine.Effect.disconnectInput>` functions.

If you need to destroy permanently the Effect, just call :func:`destroy() <NatronEngine.Effect.destroy()>`.

For convenience some GUI functionalities have been made accessible via the Effect class
to control the GUI of the node (on the node graph):

    * Get/Set the node position with the :func:`setPosition(x,y)<NatronEngine.Effect.setPosition>` and :func:`getPosition()<NatronEngine.Effect.getPosition>` functions
    * Get/Set the node size with the :func:`setSize(width,height)<NatronEngine.Effect.setSize>` and :func:`getSize()<NatronEngine.Effect.getSize>` functions
    * Get/Set the node color with the :func:`setColor(r,g,b)<NatronEngine.Effect.setColor>` and :func:`getColor()<NatronEngine.Effect.getColor>` functions

.. _userParams:

Creating user parameters
^^^^^^^^^^^^^^^^^^^^^^^^

See :ref:`this section<userParams.details>`

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Effect.addUserPlane(planeName,channels)

    :param planeName: :class:`str<NatronEngine.std::string>`
    :param channels: :class:`sequence`
    :rtype: :class:`bool<PySide.QtCore.bool>`

    Adds a new plane to the Channels selector of the node in its settings panel. When selected,
    the end-user can choose to output the result of the node to this new custom plane.
    The *planeName* will identify the plane uniquely and must not contain spaces or non
    python compliant characters.
    The *channels* are a sequence of channel names, e.g.:

        addUserPlane("MyLayer",["R", "G", "B", "A"])

    .. note::

        A plane cannot contain more than 4 channels and must at least have 1 channel.

    This function returns *True* if the layer was added successfully, *False* otherwise.

.. method:: NatronEngine.Effect.beginChanges()

    Starts a begin/End bracket, blocking all evaluation (=renders and callback onParamChanged) that would be issued due to
    a call to :func:`setValue<NatronEngine.IntParam.setValue>` on any parameter of the Effect.

    Similarly all input changes will not be evaluated until endChanges() is called.

    Typically to change several values at once we bracket the changes like this::

        node.beginChanges()
        param1.setValue(...)
        param2.setValue(...)
        param3.setValue(...)
        param4.setValue(...)
        node.endChanges()  # This triggers a new render

    A more complex call:

        node.beginChanges()
        node.connectInput(0,otherNode)
        node.connectInput(1,thirdNode)
        param1.setValue(...)
        node.endChanges() # This triggers a new render


.. method:: NatronEngine.Effect.endChanges()

    Ends a begin/end bracket. If the begin/end bracket recursion reaches 0 and there were calls
    made to :func:`setValue<NatronEngine.IntParam.setValue>` this function will effectively compresss
    all evaluations into a single one.
    See :func:`beginChanges()<NatronEngine.Effect.beginChanges>`

.. method:: NatronEngine.Effect.beginParametersUndoCommand (commandName)

    :param commandName: :class:`str<PySide.QtCore.QString>`

Same as :func:`beginChanges()<NatronEngine.Effect.beginChanges>` except that all
parameter changes are gathered under the same undo/redo command and the user will be able
to undo them all at once from the Edit menu. The *commandName* parameter is the text that
will be displayed in the Edit menu.


.. method:: NatronEngine.Effect.endParametersUndoCommand ()

Close a undo/redo command that was previously opened with :func:`beginParametersUndoCommand()<NatronEngine.Effect.beginParametersUndoCommand>`.

.. method:: NatronEngine.Effect.canConnectInput(inputNumber, node)


    :param inputNumber: :class:`int<PySide.QtCore.int>`
    :param node: :class:`Effect<NatronEngine.Effect>`
    :rtype: :class:`bool<PySide.QtCore.bool>`


Returns whether the given *node* can be connected at the given *inputNumber* of this
Effect. This function could return False for one of the following reasons:

    * The Effect already has an input at the given *inputNumber*
    * The *node* is None
    * The given *inputNumber* is out of range
    * The *node* cannot have any node connected to it (such as a BackDrop or an Output)
    * This Effect or the given *node* is a child of another node (for trackers only)
    * Connecting *node* would create a cycle in the graph implying that it would create infinite recursions


.. method:: NatronEngine.Effect.connectInput(inputNumber, input)


    :param inputNumber: :class:`int<PySide.QtCore.int>`
    :param input: :class:`Effect<NatronEngine.Effect>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Connects *input* to the given *inputNumber* of this Effect.
This function calls internally :func:`canConnectInput()<NatronEngine.Effect.canConnectInput>`
to determine if a connection is possible.

.. method:: NatronEngine.Effect.insertParamInViewerUI (parameter[, index=-1])

    :param parameter: :class:`Param<NatronEngine.Param>`
    :param index: :class:`int<PySide.QtCore.int>`

    Inserts the given **parameter** in the Viewer interface of this Effect.
    If **index** is -1, the parameter will be added *after* any other parameter in the Viewer
    interface, otherwise it will be inserted at the given position.

.. method:: NatronEngine.Effect.removeParamFromViewerUI (parameter)

    :param parameter: :class:`Param<NatronEngine.Param>`

    Removes the given **parameter** from the Viewer interface of this Effect.

.. method:: NatronEngine.Effect.clearViewerUIParameters ()

    Removes all parameters from the Viewer interface of this Effect.

.. method:: NatronEngine.Effect.destroy([autoReconnect=true])


    :param autoReconnect: :class:`bool<PySide.QtCore.bool>`

Removes this Effect from the current project definitively.
If *autoReconnect* is True then any nodes connected to this node will try to connect
their input to the input of this node instead.



.. method:: NatronEngine.Effect.disconnectInput(inputNumber)


    :param inputNumber: :class:`int<PySide.QtCore.int>`

Removes any input Effect connected to the given *inputNumber* of this node.


.. method:: NatronEngine.Effect.getAvailableLayers(inputNumber)

    :param inputNumber: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`sequence`

    Returns the layers available for the given *inputNumber*.
    This is a list of :ref:`ImageLayer<NatronEngine.ImageLayer>`.
    Note that if passing *-1* then this function will return the layers available in the
    *main* input in addition to the layers produced by this node.


.. method:: NatronEngine.Effect.getBitDepth()

    :rtype: :class:`ImageBitDepthEnum<NatronEngine.Natron.ImageBitDepthEnum>`

    Returns the bit-depth of the image in output of this node.

.. method:: NatronEngine.Effect.getColor()

    :rtype: :class:`tuple`

Returns the color of this node as it appears on the node graph as [R,G,B] 3-dimensional tuple.


.. method:: NatronEngine.Effect.getContainerGroup()

    :rtype: :class:`Group<NatronEngine.Group>`


    If this node is a node inside the top-level node-graph of the application, this returns
    the *app* object (of class :ref:`App<App>`). Otherwise if this node is a child of a
    group node, this will return the :ref:`Effect<Effect>` object of the group node.



.. method:: NatronEngine.Effect.getCurrentTime()


    :rtype: :class:`int<PySide.QtCore.int>`


Returns the current time of timeline if this node is currently rendering, otherwise
it returns the current time at which the node is currently rendering for the caller
thread.

.. method:: NatronEngine.Effect.getOutputFormat()

    :rtype: :class:`RectI<NatronEngine.RectI>`

    Returns the output format of this node in pixel units.


.. method:: NatronEngine.Effect.getFrameRate()

    :rtype: :class:`float<PySide.QtCore.float>`

    Returns the frame-rate of the sequence in output of this node.

.. method:: NatronEngine.Effect.getInput(inputNumber)


    :param inputNumber: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`Effect<NatronEngine.Effect>`

    Returns the node connected at the given *inputNumber*.


.. method:: NatronEngine.Effect.getInput(inputName)


:param inputName: :class:`str<PySide.QtCore.QString>`
:rtype: :class:`Effect<NatronEngine.Effect>`

    Same as :func:`getInput(inputNumber)<NatronEngine.Effect.getInput>` except that the parameter in input
    is the name of the input as diplayed on the node-graph. This function is made available for convenience.



.. method:: NatronEngine.Effect.getLabel()


    :rtype: :class:`str<NatronEngine.std::string>`

Returns the *label* of the node. See :ref:`this section<autoVar>` for a discussion
of the *label* vs the *script-name*.

.. method:: NatronEngine.Effect.getInputLabel(inputNumber)


    :param inputNumber: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`str<NatronEngine.std::string>`

Returns the label of the input at the given *inputNumber*.
It corresponds to the label displayed on the arrow of the input in the node graph.

.. method:: NatronEngine.Effect.getMaxInputCount()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the number of inputs for the node. Graphically this corresponds to the number
of arrows in input.




.. method:: NatronEngine.Effect.getParam(name)


    :param name: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Param<Param>`


Returns a :doc:`parameter<Param>` by its script-name or None if
no such parameter exists.



.. method:: NatronEngine.Effect.getParams()


    :rtype: :class:`sequence`

Returns all the :class:`Param<NatronEngine.Param>` of this Effect as a sequence.




.. method:: NatronEngine.Effect.getPluginID()


    :rtype: :class:`str<NatronEngine.std::string>`


Returns the ID of the plug-in that this node instantiate.



.. method:: NatronEngine.Effect.getPosition()


    :rtype: :class:`tuple`

Returns the current position of the node on the node-graph. This is a 2
dimensional [X,Y] tuple.
Note that in background mode, if used, this function will always return [0,0] and
should NOT be used.


.. method:: NatronEngine.Effect.getPremult()

    :rtype: :class:`ImagePremultiplicationEnum<NatronEngine.Natron.ImagePremultiplicationEnum>`

    Returns the alpha premultiplication state of the image in output of this node.

.. method:: NatronEngine.Effect.getPixelAspectRatio()

    :rtype: :class:`float<PySide.QtCore.float>`

    Returns the pixel aspect ratio of the image in output of this node.



.. method:: NatronEngine.Effect.getRegionOfDefinition(time,view)

    :param time: :class:`float<PySide.QtCore.float>`
    :param view: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`RectD<NatronEngine.RectD>`

Returns the bounding box of the image produced by this effect in canonical coordinates.
This is exactly the value displayed in the "Info" tab of the settings panel of the node
for the "Output".
This can be useful for example to set the position of a point parameter to the center
of the region of definition.

.. method:: NatronEngine.Effect.getItemsTable(tableName)

    :param tableName: :class:`str<PySide.QtCore.QString>`
    :rtype: :class:`ItemsTable<NatronEngine.ItemsTable>`

Returns the items table matching the given *tableName*.
An :ref:`ItemsTable<ItemsTable>` is used for example in the Tracker node to display the tracks or in the RotoPaint node to display
the shapes and strokes in the properties panel.

.. method:: NatronEngine.Effect.getItemsTable()

    :rtype: :class:`PyList`

Returns a list of all :ref:`ItemsTable<ItemsTable>`held by the node.


.. method:: NatronEngine.Effect.getScriptName()


    :rtype: :class:`str<NatronEngine.std::string>`


Returns the script-name of this Effect. See :ref:`this<autoVar>` section for more
information about the script-name.



.. method:: NatronEngine.Effect.getSize()

    :rtype: :class:`tuple`

Returns the size of this node on the node-graph as a 2 dimensional [Width,Height] tuple.
Note that calling this function will in background mode will always return [0,0] and
should not be used.





.. method:: NatronEngine.Effect.getUserPageParam()


    :rtype: :class:`PageParam<NatronEngine.PageParam>`


Convenience function to return the user page parameter if this Effect has one.

.. method:: NatronEngine.Effect.isNodeActivated()


    :rtype: :class:`bool<PySide.QtCore.bool>`


    Returns whether the node is activated or not.
    When deactivated, the user cannot interact with the node.
    A node is in a deactivated state after the user removed it from the node-graph:
    it still lives a little longer so that an undo operation can insert it again in the nodegraph.
    This state has nothing to do with the "Disabled" parameter in the "Node" tab of the settings panel.


.. method:: NatronEngine.Effect.isUserSelected()


    :rtype: :class:`bool<PySide.QtCore.bool>`


    Returns true if this node is selected in its containing nodegraph.


.. method:: NatronEngine.Effect.isReaderNode()

    :rtype: :class:`bool<PySide.QtCore.bool>`


    Returns True if this node is a reader node



.. method:: NatronEngine.Effect.isWriterNode()

    :rtype: :class:`bool<PySide.QtCore.bool>`


    Returns True if this node is a writer node

.. method:: NatronEngine.Effect.setColor(r, g, b)


    :param r: :class:`float<PySide.QtCore.double>`
    :param g: :class:`float<PySide.QtCore.double>`
    :param b: :class:`float<PySide.QtCore.double>`

Set the color of the node as it appears on the node graph.
Note that calling this function will in background mode will do nothing and
should not be used.



.. method:: NatronEngine.Effect.setLabel(name)


    :param name: :class:`str<NatronEngine.std::string>`

Set the label of the node as it appears in the user interface.
See :ref:`this<autoVar>` section for an explanation of the difference between the *label* and the
*script-name*.



.. method:: NatronEngine.Effect.setPosition(x, y)


    :param x: :class:`float<PySide.QtCore.double>`
    :param y: :class:`float<PySide.QtCore.double>`


Set the position of the node as it appears on the node graph.
Note that calling this function will in background mode will do nothing and
should not be used.



.. method:: NatronEngine.Effect.setScriptName(scriptName)


    :param scriptName: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Set the script-name of the node as used internally by Natron.
See :ref:`this<autoVar>` section for an explanation of the difference between the *label* and the
*script-name*.

.. warning::

    Using this function will remove any previous variable declared using the
    old script-name and will create a new variable with the new script name if valid.

If your script was using for instance a node named::

    app1.Blur1

and you renamed it BlurOne, it should now be available to Python this way::

    app1.BlurOne

but using app1.Blur1 would report the following error::

    Traceback (most recent call last):
    File "<stdin>", line 1, in <module>
    NameError: name 'Blur1' is not defined




.. method:: NatronEngine.Effect.setSize(w, h)


    :param w: :class:`float<PySide.QtCore.double>`
    :param h: :class:`float<PySide.QtCore.double>`

Set the size of the node as it appears on the node graph.
Note that calling this function will in background mode will do nothing and
should not be used.

.. method:: NatronEngine.Effect.setSubGraphEditable(editable)

    :param editable: :class:`bool<PySide.QtCore.bool>`

Can be called to disable editing of the group via Natron's graphical user interface.
This is handy to prevent users from accidentally breaking the sub-graph.
This can always be reverted by editing the python script associated.
The user will still be able to see the internal node graph but will not be able to
unlock it.


.. method:: NatronEngine.Effect.setPagesOrder(pages)

    :param pages: :class:`sequence`

Given the string list *pages* try to find the corresponding pages by their-script name
and order them in the given order.


.. method:: NatronEngine.Effect.registerOverlay (overlay, params)

    :param overlay: :class:`PyOverlayInteract<NatronEngine.PyOverlayInteract>`
    :param params: :class:`PyDict`

    This function takes in parameter a :ref:`PyOverlayInteract<NatronEngine.PyOverlayInteract>`
    and registers it as an overlay that will be drawn on the viewer when
    this node settings panel is opened.

    The key of the *params* dict must match a key in the overlay's parameters description
    returned by the function :func:`getDescription()<NatronEngine.PyOverlayInteract.getDescription>`
    of the :ref:`PyOverlayInteract<NatronEngine.PyOverlayInteract>`.
    The value associated to the key is the script-name of a parameter on this effect that
    should fill the role description returned by getDescription() on the overlay.

    Note that overlays for a node will be drawn in the order they were registered by this function.
    To re-order them, you may call :func:`removeOverlay()<NatronEngine.Effect.removeOverlay>`
    and this function again.

    If a non-optional parameter returned by the :func:`getDescription()<NatronEngine.PyOverlayInteract.getDescription>`
    is not filled with one of the parameter provided by the *params* or their type/dimension
    do not match, this function will report an error.

    For instance, to register a point parameter interact::

        # Let's create a group node
        group = app.createNode("fr.inria.built-in.Group")

        # Create a Double2D parameter that serve as a 2D point
        param = group.createDouble2DParam("point","Point")
        group.refreshUserParamsGUI()

        # Create a point interact for the parameter
        interact = PyPointOverlayInteract()

        # The PyPointOverlayInteract descriptor requires at least a single Double2DParam
        # that serve as a "position" role. Map it against the parameter we just created
        # Note that we reference the "point" parameter by its script-name
        interactParams = {"position": "point"}

        # Register the overlay on the group, it will now be displayed on the viewer
        group.registerOverlay(interact, interactParams)




.. method:: NatronEngine.Effect.removeOverlay (overlay)

    :param overlay: :class:`PyOverlayInteract<NatronEngine.PyOverlayInteract>`

    Remove an overlay previously registered with registerOverlay
