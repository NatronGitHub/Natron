.. module:: NatronEngine
.. _App:

App
***

**Inherits** :doc:`Group`

**Inherited by:** :doc:`../NatronGui/GuiApp`

Synopsis
--------

The App object represents one instance of a project.
See :ref:`detailed<app.details>` description...

Functions
^^^^^^^^^

- def :meth:`addProjectLayer<NatronEngine.App.addProjectLayer>` (layer)
- def :meth:`addFormat<NatronEngine.App.addFormat>` (formatSpec)
- def :meth:`createNode<NatronEngine.App.createNode>` (pluginID[, majorVersion=-1[, group=None] [, properties=None]])
- def :meth:`createReader<NatronEngine.App.createReader>` (filename[, group=None] [, properties=None])
- def :meth:`createWriter<NatronEngine.App.createWriter>` (filename[, group=None] [, properties=None])
- def :meth:`getAppID<NatronEngine.App.getAppID>` ()
- def :meth:`getProjectParam<NatronEngine.App.getProjectParam>` (name)
- def :meth:`getViewNames<NatronEngine.App.getViewNames>` ()
- def :meth:`render<NatronEngine.App.render>` (effect,firstFrame,lastFrame[,frameStep])
- def :meth:`render<NatronEngine.App.render>` (tasks)
- def :meth:`saveTempProject<NatronEngine.App.saveTempProject>` (filename)
- def :meth:`saveProject<NatronEngine.App.saveProject>` (filename)
- def :meth:`saveProjectAs<NatronEngine.App.saveProjectAs>` (filename)
- def :meth:`loadProject<NatronEngine.App.loadProject>` (filename)
- def :meth:`resetProject<NatronEngine.App.resetProject>` ()
- def :meth:`closeProject<NatronEngine.App.closeProject>` ()
- def :meth:`newProject<NatronEngine.App.newProject>` ()
- def :meth:`timelineGetLeftBound<NatronEngine.App.timelineGetLeftBound>` ()
- def :meth:`timelineGetRightBound<NatronEngine.App.timelineGetRightBound>` ()
- def :meth:`timelineGetTime<NatronEngine.App.timelineGetTime>` ()
- def :meth:`writeToScriptEditor<NatronEngine.App.writeToScriptEditor>` (message)

.. _app.details:

Detailed Description
--------------------

An App object is created automatically every times a new project is opened. For each
instance of Natron opened, there's a new instance of App.
You never create an App object by yourself, instead you can access them with variables
that Natron pre-declared for you: The first instance will be named app1, the second app2,etc...
See :ref:`this section<autoVar>` for an explanation of auto-declared variables.

When in background mode, (interpreter or render mode) there will always ever be a single
App instance, so Natron will make the following assignment before running any other script::

    app = app1

So you don't have to bother on which instance you're in. For :doc:`Group` Python plug-ins exported
from Natron, they have a function with the following signature::

    def createInstance(app,group):

So you don't have to bother again on which App instance your script is run.
You should only ever need to refer to the *app1*, *app2*... variables when using the
Script Editor.

Finally, you can always access the App object of any instance by calling the following function
when your script is for command line (background mode)::

    natron.getInstance(index)

Or the following function when you want to use GUI functionalities::

    natron.getGuiInstance(index)

.. warning::

    Note that in both cases, *index* is a 0-based number. So to retrieve *app1* you would
    need to call the function with *index = 0*.

Creating nodes
^^^^^^^^^^^^^^

The App object is responsible for creating new nodes. To create a node, you need to specify
which plug-in you want to instantiate and optionally specify which major version should your
node instantiate if the plug-in has multiple versions.
For instance we could create a new Reader node this way::

    reader = app.createNode("fr.inria.openfx.ReadOIIO")

You can also specify the group into which the node should be created, None being the project's
top level::

    group = app.createNode("fr.inria.built-in.Group")
    reader = app.createNode("fr.inria.openfx.ReadOIIO", -1, group)

For convenience, small wrapper functions have been made to directly create a Reader or Writer
given a filename::

    reader = app.createReader("/Users/me/Pictures/mySequence###.exr")
    writer = app.createWriter("/Users/me/Pictures/myVideo.mov")

In case 2 plug-ins can decode/encode the same format, e.g. ReadPSD and ReadOIIO can both
read .psd files, internally Natron picks the "best" OpenFX plug-in to decode/encode the image sequence/video
according to the settings in the Preferences of Natron.
If however you need a specific decoder/encoder to decode/encode the file format, you can use
the :func:`getSettings()<NatronEngine.App.createNode>` function with the exact plug-in ID.

In Natron you can call the  following function to get a sequence with all plug-in IDs currently available::

    natron.getPluginIDs()

You can also get a sub-set of those plug-ins with the :func:`getPluginIDs(filter)<NatronEngine.PyCoreApplication.getPluginIDs>`
which returns only plug-in IDs containing the given filter (compared without case sensitivity).



Accessing the settings of Natron
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To modify the parameters in the *Preferences* of Natron, you can call the
:func:`getSettings()<NatronEngine.App.getSettings>` function to get an object
containing all the :doc:`parameters<Param>` of the preferences.

Accessing the project settings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can get a specific :doc:`parameter<Param>` of the project settings with the
:func:`getProjectParam(name)<NatronEngine.App.getProjectParam>` function.



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.App.addProjectLayer(layer)

    :param layer: :class:`ImageLayer<NatronEngine.ImageLayer>`

Appends a new project-wide layer. It will be available to all layer menus of all nodes.
Each layer menu must be refreshed individually with either a right click on the menu or
by changing nodes connections to get access to the new layer. Layer names are unique:
even if you add duplicates to the layers list, only the first one in the list with that name
will be available in the menus.

.. method:: NatronEngine.App.addFormat(formatSpec)

    :param formatSpec: :class:`str<NatronEngine.std::string>`

Attempts to add a new format to the project's formats list. The *formatSpec* parameter
must follow this spec: First the name of the format, without any spaces and without any
non Python compliant characters; followed by a space and then the size of the format, in
the form *width*x*height*; followed by a space and then the pixel aspect ratio of the
format. For instance::

    HD 1920x1080 1

Wrongly formatted format will be omitted and a warning will be printed in the *ScriptEditor*.

.. method:: NatronEngine.App.createNode(pluginID[, majorVersion=-1[, group=None] [, properties=None]])


    :param pluginID: :class:`str<NatronEngine.std::string>`
    :param majorVersion: :class:`int<PySide.QtCore.int>`
    :param group: :class:`Group<NatronEngine.Group>`
    :param properties: :class:`Dict`
    :rtype: :class:`Effect<NatronEngine.Effect>`

Creates a new node instantiating the plug-in specified with the given *pluginID* at the given
*majorVersion*. If *majorVersion* is -1, the highest version of the plug-in will be instantiated.
The optional *group* parameter can be used to specify into which :doc:`group<Group>` the node
should be created, *None* meaning the project's top level.

In Natron you can call the  following function to get a sequence with all plug-in IDs currently available::

    natron.getPluginIDs()

The optional parameter *properties* is a dictionary containing properties that may modify
the creation of the node, such as hiding the node GUI, disabling auto-connection in the
NodeGraph, etc...

The properties are values of type Bool, Int, Float or String and are mapped against a unique
*key* identifying them.

Most properties have a default value and don't need to be specified, except the pluginID property.

Below is a list of all the properties available that are recognized by Natron. If you specify
an unknown property, Natron will print a warning in the Script Editor.

All properties type have been wrapped to Natron types:

- A boolean property is represented by the **BoolNodeCreationProperty** class
- An int property is represented by the **IntNodeCreationProperty** class
- A float property is represented by the **FloatNodeCreationProperty** class
- A string property is represented by the **StringNodeCreationProperty** class

Here is an example on how to pass properties to the createNode function::

    app.createNode("net.sf.cimg.CImgBlur", -1, app, dict([ ("CreateNodeArgsPropSettingsOpened", NatronEngine.BoolNodeCreationProperty(True)), ("CreateNodeArgsPropNodeInitialParamValues", NatronEngine.StringNodeCreationProperty("size")) ,("CreateNodeArgsPropParamValue_size",NatronEngine.FloatNodeCreationProperty([2.3,5.1])) ]))



- *Name*: **CreateNodeArgsPropPluginID**

  *Dimension*: 1

  *Type*: string

  *Default*: None

  *Description*: Indicates the ID of the plug-in to create. This property is mandatory.
  It is set automatically by passing the pluginID to the createNode function

- *Name*: **CreateNodeArgsPropPluginVersion**

  *Dimension*: 2

  *Type*: int

  *Default*: -1,-1

  *Description*: Indicates the version of the plug-in to create.
  With the value (-1,-1) Natron will load the highest possible version available for that plug-in.

- *Name*: **CreateNodeArgsPropNodeInitialPosition**

  *Dimension*: 2

  *Type*: float

  *Default*: None

  *Description*: Indicates the initial position of the node in the nodegraph.
  By default Natron will position the node according to the state of the interface (current selection, position of the viewport, etc...)

- *Name*: **CreateNodeArgsPropNodeInitialName**

  *Dimension*: 1

  *Type*: string

  *Default*: None

  *Description*: Indicates the initial *script-name* of the node
  By default Natron will name the node according to the plug-in label and will add a digit
  afterwards dependending on the current number of instances of that plug-in.

- *Name*: **CreateNodeArgsPropNodeInitialParamValues**

  *Dimension*: N

  *Type*: string

  *Default*: None

  *Description*: Contains a sequence of parameter script-names for which a default value
  is specified by a property. Each default value must be specified by a property whose name is
  in the form *CreateNodeArgsPropParamValue_PARAMETERNAME*  where *PARAMETERNAME* must be replaced by the
  *script-name* of the parameter.  The property must have the same type as the data-type of
  the parameter (e.g. int for IntParam, float for FloatParam, bool for BooleanParam, String for StringParam).


- *Name*: **CreateNodeArgsPropOutOfProject**

  *Dimension*: 1

  *Type*: bool

  *Default*: False

  *Description*: When True the node will not be part of the project. The node can be used for internal used, e.g. in a Python script but will
  not appear to the user. It will also not be saved in the project.


- *Name*: **CreateNodeArgsPropNoNodeGUI**

  *Dimension*: 1

  *Type*: bool

  *Default*: False

  *Description*:  * If True, the node will not have any GUI created. The property CreateNodeArgsPropOutOfProject set to True implies this.


- *Name*: **CreateNodeArgsPropSettingsOpened**

  *Dimension*: 1

  *Type*: bool

  *Default*: False

  *Description*:  * If True, the node settings panel will not be opened by default when created.
  If the property CreateNodeArgsPropNoNodeGUI is set to true or CreateNodeArgsPropOutOfProject
  is set to true, this property has no effet.


- *Name*: **CreateNodeArgsPropAutoConnect**

  *Dimension*: 1

  *Type*: bool

  *Default*: False

  *Description*:  * If True, Natron will try to automatically connect the node to others depending on the user selection.
  If the property CreateNodeArgsPropNoNodeGUI is set to true or CreateNodeArgsPropOutOfProject
  is set to true, this property has no effet.


- *Name*: **CreateNodeArgsPropAddUndoRedoCommand**

  *Dimension*: 1

  *Type*: bool

  *Default*: False

  *Description*:  Natron will push a undo/redo command to the stack when creating this node.
  If the property CreateNodeArgsPropNoNodeGUI is set to true or CreateNodeArgsPropOutOfProject
  is set to true, this property has no effect.


- *Name*: **CreateNodeArgsPropSilent**

  *Dimension*: 1

  *Type*: bool

  *Default*: True

  *Description*:  When set to True, Natron will not show any information, error, warning, question or file dialog when creating the node.



.. method:: NatronEngine.App.createReader(filename[, group=None] [, properties=None])


    :param filename: :class:`str<NatronEngine.std::string>`
    :param group: :class:`Group<NatronEngine.Group>`
    :rtype: :class:`Effect<NatronEngine.Effect>`

Creates a reader to decode the given *filename*.
The optional *group* parameter can be used to specify into which :doc:`group<Group>` the node
should be created, *None* meaning the project's top level.

In case 2 plug-ins can decode the same format, e.g. ReadPSD and ReadOIIO can both
read .psd files, internally Natron picks the "best" OpenFX plug-in to decode the image sequence/video
according to the settings in the Preferences of Natron.
If however you need a specific decoder to decode the file format, you can use
the :func:`getSettings()<NatronEngine.App.createNode>` function with the exact plug-in ID.


.. method:: NatronEngine.App.createWriter(filename[, group=None] [, properties=None])


    :param filename: :class:`str<NatronEngine.std::string>`
    :param group: :class:`Group<NatronEngine.Group>`
    :rtype: :class:`Effect<NatronEngine.Effect>`

Creates a reader to decode the given *filename*.
The optional *group* parameter can be used to specify into which :doc:`group<Group>` the node
should be created, *None* meaning the project's top level.

In case 2 plug-ins can encode the same format, e.g. WritePFM and WriteOIIO can both
write .pfm files, internally Natron picks the "best" OpenFX plug-in to encode the image sequence/video
according to the settings in the Preferences of Natron.
If however you need a specific decoder to encode the file format, you can use
the :func:`getSettings()<NatronEngine.App.createNode>` function with the exact plug-in ID.

.. method:: NatronEngine.App.getAppID()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the **zero-based** ID of the App instance.
*app1* would have the AppID 0, *app2* would have the AppID 1, and so on...




.. method:: NatronEngine.App.getProjectParam(name)


    :param name: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Param<NatronEngine.Param>`

Returns a project :doc:`Param` given its *name* (script-name). See :ref:`this section<autoVar>` for
an explanation of *script-name* vs. *label*.


.. method:: NatronEngine.App.getViewNames()

    :rtype: :class:`Sequence`

Returns a sequence with the name of all the views in the project as setup by the user
in the "Views" tab of the Project Settings.

.. method:: NatronEngine.App.render(effect,firstFrame,lastFrame[,frameStep])


    :param effect: :class:`Effect<NatronEngine.Effect>`

    :param firstFrame: :class:`int<PySide.QtCore.int>`

    :param lastFrame: :class:`int<PySide.QtCore.int>`

    :param frameStep: :class:`int<PySide.QtCore.int>`


Starts rendering the given *effect* on the frame-range defined by [*firstFrame*,*lastFrame*].
The *frameStep* parameter indicates how many frames the timeline should step after rendering
each frame. The value must be greater or equal to 1.
The *frameStep* parameter is optional and if not given will default to the value of the
**Frame Increment** parameter in the Write node.

For instance::

    render(effect,1,10,2)

Would render the frames 1,3,5,7,9


This is a blocking function only in background mode.
A blocking render means that this function returns only when the render finishes (from failure or success).

This function should only be used to render with a Write node or DiskCache node.


.. method:: NatronEngine.App.render(tasks)


    :param tasks: :class:`sequence`

This function takes a sequence of tuples of the form *(effect,firstFrame,lastFrame[,frameStep])*
The *frameStep* is optional in the tuple and if not set will default to the value of the
**Frame Increment** parameter in the Write node.

This is an overloaded function. Same as :func:`render(effect,firstFrame,lastFrame,frameStep)<NatronEngine.App.render>`
but all *tasks* will be rendered concurrently.

This function is called when rendering a script in background mode with
multiple writers.

This is a blocking call only in background mode.



.. method:: NatronEngine.App.timelineGetLeftBound()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the *left bound* of the timeline, that is, the first member of the project's frame-range parameter




.. method:: NatronEngine.App.timelineGetRightBound()


    :rtype: :class:`int<PySide.QtCore.int>`


Returns the *right bound* of the timeline, that is, the second member of the project's frame-range parameter



.. method:: NatronEngine.App.timelineGetTime()


    :rtype: :class:`int<PySide.QtCore.int>`

Get the timeline's current time.
In Natron there's only a single internal timeline and all Viewers are synchronised on that
timeline. If the user seeks a specific frames, then all Viewers will render that frame.


.. method:: NatronEngine.App.writeToScriptEditor(message)

    :param message: :class:`str<NatronEngine.std::string>`

Writes the given *message* to the Script Editor panel of Natron. This can be useful to
inform the user of various information, warnings or errors.


.. method:: NatronEngine.App.saveProject(filename)

    :param filename: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

    Saves the current project under the current project name. If the project has
    never been saved so far, this function e saves the project to the file indicated by the *filename*
    parameter. In GUI mode, if *filename* is empty, it asks the user where to save the project in GUI
    mode.

    This function returns *True* if it saved successfully, *False* otherwise.

.. method:: NatronEngine.App.saveProjectAs(filename)

    :param filename: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

    Save the project under the given *filename*.
    In GUI mode, if *filename* is empty, it prompts the user where to save the project.


    This function returns *True* if it saved successfully, *False* otherwise.



.. method:: NatronEngine.App.saveTempProject(filename)

    :param filename: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

    Saves a copy of the project to the given *filename* without updating project properties
    such as the project path, last save time etc...
    This function returns *True* if it saved successfully, *False* otherwise.


.. method:: NatronEngine.App.loadProject(filename)

    :param filename: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`App<NatronEngine.App>`

    Loads the project indicated by *filename*.
    In GUI mode, this will open a new window only if the current window has modifications.
    In background mode this will close the current project of this :class:`App<NatronEngine.App>`
    and open the project indicated by *filename* in it.
    This function returns the :class:`App<NatronEngine.App>` object upon success, *None* otherwise.


.. method:: NatronEngine.App.resetProject()

    :rtype: :class:`bool<PySide.QtCore.bool>`

    Attempts to close the current project, without wiping the window.
    In GUI mode, the user is first prompted to saved his/her changes and can abort the
    reset, in which case this function will return *False*.
    In background mode this function always succeeds, hence always returns *True*.
    this always succeed.

.. method:: NatronEngine.App.closeProject()

    :rtype: :class:`bool<PySide.QtCore.bool>`

    Same as :func:`resetProject()<NatronEngine.App.resetProject>` except that the
    window will close in GUI mode.
    Also, if this is the last :class:`App<NatronEngine.App>` alive, Natron will close.

.. method:: NatronEngine.App.newProject()

    :rtype: :class:`App<NatronEngine.App>`

    Creates a new :class:`App<NatronEngine.App>`. In GUI mode, this will open a new window.
    Upon success, the :class:`App<NatronEngine.App>` object is returned, otherwise *None*
    is returned.

