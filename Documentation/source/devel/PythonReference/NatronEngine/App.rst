.. currentmodule:: NatronEngine
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
- def :meth:`beginUndoGroup<NatronEngine.App.beginUndoGroup>` (text)
- def :meth:`canRedo<NatronEngine.App.canRedo>` ()
- def :meth:`canUndo<NatronEngine.App.canUndo>` ()
- def :meth:`createNode<NatronEngine.App.createNode>` (pluginID[, majorVersion=-1[, group=None] [, properties=None]])
- def :meth:`createReader<NatronEngine.App.createReader>` (filename[, group=None] [, properties=None])
- def :meth:`createWriter<NatronEngine.App.createWriter>` (filename[, group=None] [, properties=None])
- def :meth:`endUndoGroup<NatronEngine.App.endUndoGroup>` ()
- def :meth:`getAppID<NatronEngine.App.getAppID>` ()
- def :meth:`getProjectParam<NatronEngine.App.getProjectParam>` (name)
- def :meth:`getUndoHistory<NatronEngine.App.getUndoHistory>` ()
- def :meth:`getUndoIndex<NatronEngine.App.getUndoIndex>` ()
- def :meth:`getViewNames<NatronEngine.App.getViewNames>` ()
- def :meth:`redo<NatronEngine.App.redo>` ()
- def :meth:`render<NatronEngine.App.render>` (effect,firstFrame,lastFrame[,frameStep])
- def :meth:`render<NatronEngine.App.render>` (tasks)
- def :meth:`saveTempProject<NatronEngine.App.saveTempProject>` (filename[, format=NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatNone])
- def :meth:`saveProject<NatronEngine.App.saveProject>` (filename[, format=NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatNone])
- def :meth:`saveProjectAs<NatronEngine.App.saveProjectAs>` (filename[, format=NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatNone])
- def :meth:`getProjectFormat<NatronEngine.App.getProjectFormat>` ()
- def :meth:`setProjectFormat<NatronEngine.App.setProjectFormat>` (format)
- def :meth:`projectFileHasGuiData<NatronEngine.App.projectFileHasGuiData>` ()
- def :meth:`projectLoadedWithErrors<NatronEngine.App.projectLoadedWithErrors>` ()
- def :meth:`loadProject<NatronEngine.App.loadProject>` (filename)
- def :meth:`resetProject<NatronEngine.App.resetProject>` ()
- def :meth:`closeProject<NatronEngine.App.closeProject>` ()
- def :meth:`newProject<NatronEngine.App.newProject>` ()
- def :meth:`timelineGetLeftBound<NatronEngine.App.timelineGetLeftBound>` ()
- def :meth:`timelineGetRightBound<NatronEngine.App.timelineGetRightBound>` ()
- def :meth:`timelineGetTime<NatronEngine.App.timelineGetTime>` ()
- def :meth:`undo<NatronEngine.App.undo>` ()
- def :meth:`undoGroup<NatronEngine.App.undoGroup>` (text)
- def :meth:`waitForRenders<NatronEngine.App.waitForRenders>` ([timeout=-1.0])
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
:func:`natron.getSettings()<NatronEngine.PyCoreApplication.getSettings>` function to get an object
containing all the :doc:`parameters<Param>` of the preferences.

Accessing the project settings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can get a specific :doc:`parameter<Param>` of the project settings with the
:func:`getProjectParam(name)<NatronEngine.App.getProjectParam>` function.



Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: App.addProjectLayer(layer)

    :param layer: :class:`ImageLayer<NatronEngine.ImageLayer>`

Appends a new project-wide layer. It will be available to all layer menus of all nodes.
Each layer menu must be refreshed individually with either a right click on the menu or
by changing nodes connections to get access to the new layer. Layer names are unique:
even if you add duplicates to the layers list, only the first one in the list with that name
will be available in the menus.

.. method:: App.addFormat(formatSpec)

    :param formatSpec: :class:`str`

Attempts to add a new format to the project's formats list. The *formatSpec* parameter
must follow this spec: First the name of the format, without any spaces and without any
non Python compliant characters; followed by a space and then the size of the format, in
the form *width*x*height*; followed by a space and then the pixel aspect ratio of the
format. For instance::

    HD 1920x1080 1

Wrongly formatted format will be omitted and a warning will be printed in the *ScriptEditor*.

.. method:: App.createNode(pluginID[, majorVersion=-1[, group=None] [, properties=None]])


    :param pluginID: :class:`str`
    :param majorVersion: :class:`int`
    :param group: :class:`Group<NatronEngine.Group>`
    :param properties: :class:`dict`
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

All properties type have been wrapped to Natron types (see :doc:`NodeCreationProperty`):

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



.. method:: App.createReader(filename[, group=None] [, properties=None])


    :param filename: :class:`str`
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


.. method:: App.createWriter(filename[, group=None] [, properties=None])


    :param filename: :class:`str`
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

.. method:: App.getAppID()


    :rtype: :class:`int`

Returns the **zero-based** ID of the App instance.
*app1* would have the AppID 0, *app2* would have the AppID 1, and so on...




.. method:: App.getProjectParam(name)


    :param name: :class:`str`
    :rtype: :class:`Param<NatronEngine.Param>`

Returns a project :doc:`Param` given its *name* (script-name). See :ref:`this section<autoVar>` for
an explanation of *script-name* vs. *label*.


.. method:: App.getViewNames()

    :rtype: :class:`Sequence`

Returns a sequence with the name of all the views in the project as setup by the user
in the "Views" tab of the Project Settings.

.. method:: App.render(effect,firstFrame,lastFrame[,frameStep])


    :param effect: :class:`Effect<NatronEngine.Effect>`

    :param firstFrame: :class:`int`

    :param lastFrame: :class:`int`

    :param frameStep: :class:`int`


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


.. method:: App.render(tasks)
    :noindex:


    :param tasks: :class:`sequence`

This function takes a sequence of tuples of the form *(effect,firstFrame,lastFrame[,frameStep])*
The *frameStep* is optional in the tuple and if not set will default to the value of the
**Frame Increment** parameter in the Write node.

This is an overloaded function. Same as :func:`render(effect,firstFrame,lastFrame,frameStep)<NatronEngine.App.render>`
but all *tasks* will be rendered concurrently.

This function is called when rendering a script in background mode with
multiple writers.

This is a blocking call only in background mode.



.. method:: App.timelineGetLeftBound()


    :rtype: :class:`int`

Returns the *left bound* of the timeline, that is, the first member of the project's frame-range parameter




.. method:: App.timelineGetRightBound()


    :rtype: :class:`int`


Returns the *right bound* of the timeline, that is, the second member of the project's frame-range parameter



.. method:: App.timelineGetTime()


    :rtype: :class:`int`

Get the timeline's current time.
In Natron there's only a single internal timeline and all Viewers are synchronised on that
timeline. If the user seeks a specific frames, then all Viewers will render that frame.


.. method:: App.writeToScriptEditor(message)

    :param message: :class:`str`

Writes the given *message* to the Script Editor panel of Natron. This can be useful to
inform the user of various information, warnings or errors.


.. method:: App.saveProject(filename[, format=NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatNone])

    :param filename: :class:`str`
    :param format: :class:`ProjectFileFormatEnum<NatronEngine.Natron.ProjectFileFormatEnum>`
    :rtype: :class:`bool`

    Saves the current project under the current project name. If the project has
    never been saved so far, this function e saves the project to the file indicated by the *filename*
    parameter. In GUI mode, if *filename* is empty, it asks the user where to save the project in GUI
    mode.

    *format* is the format of the file, see :meth:`getProjectFormat<NatronEngine.App.getProjectFormat>`. With
    *eProjectFileFormatNone* (the default), the project is saved in its format, or, if it was never saved,
    in the format of the "Save projects as binary by default" preference (binary by default, also in
    NatronRenderer and natron-python). Any other format becomes the format of the project.
    In GUI mode, when *filename* is empty and the project was never saved, the save dialog is shown:
    *format* is the default of its "File format" choice, and the user's choice is used.

    This function returns *True* if it saved successfully, *False* otherwise.

    .. versionchanged:: 2.6
       The *format* parameter. Projects are saved in the binary format by default.

.. method:: App.saveProjectAs(filename[, format=NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatNone])

    :param filename: :class:`str`
    :param format: :class:`ProjectFileFormatEnum<NatronEngine.Natron.ProjectFileFormatEnum>`
    :rtype: :class:`bool`

    Save the project under the given *filename*.
    In GUI mode, if *filename* is empty, it prompts the user where to save the project.

    *format* is the format of the file, as in :meth:`saveProject<NatronEngine.App.saveProject>`: it becomes
    the format of the project, unless it is *eProjectFileFormatNone*.
    To write a file that Natron 2.5 and earlier can read, use
    *NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatText*.

    This function returns *True* if it saved successfully, *False* otherwise.

    .. versionchanged:: 2.6
       The *format* parameter. Projects are saved in the binary format by default.


.. method:: App.saveTempProject(filename[, format=NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatNone])

    :param filename: :class:`str`
    :param format: :class:`ProjectFileFormatEnum<NatronEngine.Natron.ProjectFileFormatEnum>`
    :rtype: :class:`bool`

    Saves a copy of the project to the given *filename* without updating project properties
    such as the project path, last save time etc...
    *format* is the format of the copy (*eProjectFileFormatNone*: the format of the project, or the
    preference for a project that was never saved); it doesn't change the format of the project.
    This function returns *True* if it saved successfully, *False* otherwise.

    .. versionchanged:: 2.6
       The *format* parameter.

.. method:: App.getProjectFormat()

    :rtype: :class:`ProjectFileFormatEnum<NatronEngine.Natron.ProjectFileFormatEnum>`

    Returns the format of the project file: the format of the file it was loaded from, or of the
    file it was last saved to. *eProjectFileFormatNone* for a project that was never saved.
    A project restored from an auto-save (auto-saves are always binary) has the format of the
    project that was auto-saved.

    Project files have the *.ntp* extension in both formats, and the format is detected when a
    project is loaded:

    - *eProjectFileFormatText*: XML text, readable and diffable, which Natron 2.5 and earlier can read.
    - *eProjectFileFormatBinary*: a portable binary format (the same file loads on every operating
      system and processor), faster to load and save, and smaller. Needs Natron 2.6 or later.

    .. versionadded:: 2.6

.. method:: App.setProjectFormat(format)

    :param format: :class:`ProjectFileFormatEnum<NatronEngine.Natron.ProjectFileFormatEnum>`

    Sets the format of the next saves of the project (see :meth:`getProjectFormat<NatronEngine.App.getProjectFormat>`).

    .. versionadded:: 2.6

.. method:: App.projectFileHasGuiData()

    :rtype: :class:`bool`

    Returns whether the project file this project was loaded from has GUI data: node positions,
    colours and sizes, viewer settings, the window layout... A project saved by the Natron GUI has
    them, a project saved by NatronRenderer or ``Natron -b`` doesn't. *False* for a new project.
    It describes the file that was loaded: saving the project doesn't change it.

    NatronRenderer and ``Natron -b`` don't read the GUI data and don't create the Viewer nodes, and
    save projects without them: a script run by them that saves a project with GUI data loses them.
    Such a script can use this function to refuse to save the project, or to warn.

    .. versionadded:: 2.6

.. method:: App.projectLoadedWithErrors()

    :rtype: :class:`bool`

    Returns whether something in the project file was not loaded, so that saving the project would
    lose it: a node that can't be created, e.g. because its plug-in is missing (also inside a
    Group), a connection to an input that the node doesn't have, a link to a master node that
    doesn't exist, or an error that stopped the load. The error log lists them.
    :meth:`loadProject<NatronEngine.App.loadProject>` returns the project in that case too, except
    after an error that stopped the load.
    Warnings where nothing is lost, such as a node created with another major version of its
    plug-in, don't make it *True*, even when they open the error log.
    *False* for a new project. It describes the file that was loaded: saving the project doesn't
    change it. It is already set when the ``afterProjectLoad`` callback runs.

    .. versionadded:: 2.6


.. method:: App.loadProject(filename)

    :param filename: :class:`str`
    :rtype: :class:`App<NatronEngine.App>`

    Loads the project indicated by *filename*.
    In GUI mode, this will open a new window only if the current window has modifications.
    In background mode this will close the current project of this :class:`App<NatronEngine.App>`
    and open the project indicated by *filename* in it.
    This function returns the :class:`App<NatronEngine.App>` object upon success, *None* otherwise.


.. method:: App.resetProject()

    :rtype: :class:`bool`

    Attempts to close the current project, without wiping the window.
    In GUI mode, the user is first prompted to saved his/her changes and can abort the
    reset, in which case this function will return *False*.
    In background mode this function always succeeds, hence always returns *True*.
    this always succeed.

.. method:: App.closeProject()

    :rtype: :class:`bool`

    Same as :func:`resetProject()<NatronEngine.App.resetProject>` except that the
    window will close in GUI mode.
    Also, if this is the last :class:`App<NatronEngine.App>` alive, Natron will close.
    In GUI mode, the window closes once the script returns to the event loop: in the meantime,
    the project is being closed, and :func:`createNode()<NatronEngine.App.createNode>` returns
    *None*, and :func:`natron.getNumInstances()<NatronEngine.PyCoreApplication.getNumInstances>`
    still counts it. A script should not use this :class:`App<NatronEngine.App>` after this call.

.. method:: App.newProject()

    :rtype: :class:`App<NatronEngine.App>`

    Creates a new :class:`App<NatronEngine.App>`. In GUI mode, this will open a new window.
    Upon success, the :class:`App<NatronEngine.App>` object is returned, otherwise *None*
    is returned.

.. method:: App.undo()

    Undoes the last command of the undo stack of the node graph: node creations, deletions,
    connections and moves, and the undo groups (see below), whatever widget has the focus. *Edit >
    Undo* undoes the last command of the panel or graph used last instead, which may be a parameter
    change in a settings panel. Does nothing if there is nothing to undo, and in background mode,
    where there is no undo stack. Raises :class:`RuntimeError` when not called from the main thread
    (e.g. from a render callback); so do the other undo functions.

    Edits made from Python (e.g. :meth:`Param.setValue()<NatronEngine.IntParam.setValue>` or
    :meth:`createNode()<NatronEngine.App.createNode>`) are only undoable inside an undo group (see
    :meth:`undoGroup()<NatronEngine.App.undoGroup>`).

.. method:: App.redo()

    Redoes the last undone command of the node graph's undo stack (see
    :meth:`undo()<NatronEngine.App.undo>`). Does nothing if there is nothing to redo, and in background
    mode.

.. method:: App.canUndo()

    :rtype: :class:`bool`

    Returns whether :meth:`undo()<NatronEngine.App.undo>` would undo something. Always *False* in
    background mode.

.. method:: App.canRedo()

    :rtype: :class:`bool`

    Returns whether :meth:`redo()<NatronEngine.App.redo>` would redo something. Always *False* in
    background mode.

.. method:: App.getUndoHistory()

    :rtype: :class:`sequence`

    Returns the texts of the commands of the node graph's undo stack, oldest first, as in the undo history
    (e.g. ``['Delete node', 'Paste node']``). Commands at an index greater than or equal to
    :meth:`getUndoIndex()<NatronEngine.App.getUndoIndex>` were undone and can be redone. Returns an empty
    list in background mode.

.. method:: App.getUndoIndex()

    :rtype: :class:`int`

    Returns the number of commands of :meth:`getUndoHistory()<NatronEngine.App.getUndoHistory>` that are
    done: :meth:`undo()<NatronEngine.App.undo>` undoes the command just before this index, and
    :meth:`redo()<NatronEngine.App.redo>` redoes the one at this index. Returns 0 in background mode.

.. method:: App.waitForRenders([timeout=-1.0])

    :param timeout: :class:`float`
    :rtype: :class:`bool`

    Waits until no render runs or is queued, in any project: the viewers, the Write nodes and the node
    previews. In the user interface, the events (except the user input) are processed while waiting,
    until the results of the renders are displayed: there it waits until no render has run for 200
    ms, so a call takes at least 200 ms. Returns *True*, or *False* if *timeout* seconds elapsed first
    (no limit if *timeout* is negative). Must be called from the main thread (e.g. a script, a callback or a
    PyPanel), not from a render callback.

    For example, a button callback that changes a parameter, then reads the result of the render::

        transform.getParam("translate").set(100, 50)
        app.getViewer("Viewer1").renderCurrentFrame()
        if app.waitForRenders(30):
            print(app.getViewer("Viewer1").getPixel(960, 540))

.. method:: App.undoGroup(text)

    :param text: :class:`str`
    :rtype: a context manager

    Makes the edits of a block of Python code one undo step named *text*, used as::

        with app.undoGroup("Bake"):
            blur = app.createNode("net.sf.cimg.CImgBlur")
            blur.connectInput(0, app.Read1)
            blur.getParam("size").setValue(10, 0)
            app.Viewer1.connectInput(0, blur)

    *Edit > Undo* (or :meth:`undo()<NatronEngine.App.undo>`) then removes the Blur node and restores the
    connection of the viewer in one step. Outside an undo group, the edits made from Python are not
    undoable, as before. This is how a PyPlug button callback or a tool makes its edits one undo step.

    The group records the parameter changes (values, keyframes, expressions, links and aliases, defaults
    restored),
    the nodes created (:meth:`createNode()<NatronEngine.App.createNode>`,
    :meth:`createReader()<NatronEngine.App.createReader>`, :meth:`createWriter()<NatronEngine.App.createWriter>`),
    deleted (:meth:`Effect.destroy()<NatronEngine.Effect.destroy>`) and connected or disconnected
    (:meth:`Effect.connectInput()<NatronEngine.Effect.connectInput>`,
    :meth:`Effect.disconnectInput()<NatronEngine.Effect.disconnectInput>`). A node deleted in a group is
    kept until the undo step is discarded, so that undo can restore it.

    Groups nest: only the outermost one makes an undo step. The group ends when the block ends, also when
    it raises an exception (the edits made until then are the undo step). The undo step is on the undo
    stack of the node graph. In background mode (NatronRenderer), where there is no undo stack, it does
    nothing.

    Undoing restores, for each parameter, its values, keyframes, expressions and links as they were
    before the group, unless the parameter was changed after the group (e.g. in its settings panel, whose
    undo stack is another one): it then keeps its value. Redo does the same the other way round.

    Not recorded: the changes that a plug-in makes by itself in reaction to an edit (e.g. a Read node's
    format after its file name changes), which happen again or not when the edit is undone or redone;
    the label, position and colour of the nodes (:meth:`Effect.setLabel()<NatronEngine.Effect.setLabel>`,
    ``setPosition()``, ``setColor()``). A node deleted in a group keeps its script name taken, so a node
    created afterwards with the same name gets another one, as after a deletion in the node graph.

    A group that a script leaves open (a :meth:`beginUndoGroup()<NatronEngine.App.beginUndoGroup>`
    without :meth:`endUndoGroup()<NatronEngine.App.endUndoGroup>`) is ended, with a warning, when the
    Script Editor script or the GUI test that started it ends, and dropped when the project is closed.
    :meth:`undo()<NatronEngine.App.undo>` and :meth:`redo()<NatronEngine.App.redo>` raise
    :class:`RuntimeError` while a group is open, and the undo group functions raise it when not called
    from the main thread.

.. method:: App.beginUndoGroup(text)

    :param text: :class:`str`

    Starts an undo group, as entering ``with app.undoGroup(text):`` does: prefer the ``with`` statement,
    which also ends the group on an exception. Every call must be followed by a call to
    :meth:`endUndoGroup()<NatronEngine.App.endUndoGroup>`.

.. method:: App.endUndoGroup()

    Ends the undo group started by the last :meth:`beginUndoGroup()<NatronEngine.App.beginUndoGroup>`.
    Raises :class:`RuntimeError` if no group is open.
