.. module:: NatronEngine
.. _App:

App
***

**Inherits** :doc:`Group`

**Inherited by:** :ref:`GuiApp`

Synopsis
--------

The App object represents one instance of a project. 
See :ref:`detailed<app.details>` description...

Functions
^^^^^^^^^

*	 def :meth:`addFormat<NatronEngine.App.addFormat>` (formatSpec)
*    def :meth:`createNode<NatronEngine.App.createNode>` (pluginID[, majorVersion=-1[, group=None]])
*    def :meth:`getAppID<NatronEngine.App.getAppID>` ()
*    def :meth:`getProjectParam<NatronEngine.App.getProjectParam>` (name)
*    def :meth:`render<NatronEngine.App.render>` (task)
*    def :meth:`render<NatronEngine.App.render>` (tasks)
*    def :meth:`saveTempProject<NatronEngine.App.saveTempProject>` (filename)
*    def :meth:`saveProject<NatronEngine.App.saveProject>` (filename)
*    def :meth:`saveProjectAs<NatronEngine.App.saveProjectAs>` (filename)
*    def :meth:`loadProject<NatronEngine.App.loadProject>` (filename)
*    def :meth:`resetProject<NatronEngine.App.resetProject>` ()
*    def :meth:`closeProject<NatronEngine.App.closeProject>` ()
*    def :meth:`newProject<NatronEngine.App.newProject>` ()
*    def :meth:`timelineGetLeftBound<NatronEngine.App.timelineGetLeftBound>` ()
*    def :meth:`timelineGetRightBound<NatronEngine.App.timelineGetRightBound>` ()
*    def :meth:`timelineGetTime<NatronEngine.App.timelineGetTime>` ()
*    def :meth:`writeToScriptEditor<NatronEngine.App.writeToScriptEditor>` (message)

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

You find it hard to know what is the plug-in ID of a plug-in ? In Natron you can call the 
following function to get a sequence with all plug-in IDs currently available::

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

.. method:: NatronEngine.App.addFormat(formatSpec)

	:param formatSpec: :class:`str<NatronEngine.std::string>`
	
Attempts to add a new format to the project's formats list. The *formatSpec* parameter
must follow this spec: First the name of the format, without any spaces and without any
non Python compliant characters; followed by a space and then the size of the format, in
the form *width*x*height*; followed by a space and then the pixel aspect ratio of the
format. For instance::

	HD 1920x1080 1 
	
Wrongly formatted format will be omitted and a warning will be printed in the *ScriptEditor*.

.. method:: NatronEngine.App.createNode(pluginID[, majorVersion=-1[, group=None]])


    :param pluginID: :class:`str<NatronEngine.std::string>`
    :param majorVersion: :class:`int<PySide.QtCore.int>`
    :param group: :class:`Group<NatronEngine.Group>`
    :rtype: :class:`Effect<NatronEngine.Effect>`

Creates a new node instantiating the plug-in specified with the given *pluginID* at the given
*majorVersion*. If *majorVersion* is -1, the highest version of the plug-in will be instantiated.
The optional *group* parameter can be used to specify into which :doc:`group<Group>` the node
should be created, *None* meaning the project's top level.

In Natron you can call the  following function to get a sequence with all plug-in IDs currently available::

	natron.getPluginIDs()



.. method:: NatronEngine.App.getAppID()


    :rtype: :class:`int<PySide.QtCore.int>`

Returns the **zero-based** ID of the App instance.
*app1* would have the AppID 0, *app2* would have the AppID 1, and so on...




.. method:: NatronEngine.App.getProjectParam(name)


    :param name: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Param<NatronEngine.Param>`

Returns a project :doc:`Param` given its *name* (script-name). See :ref:`this section<autoVar>` for 
an explanation of *script-name* vs. *label*. 




.. method:: NatronEngine.App.render(task)


    :param task: :class:`RenderTask<NatronEngine.RenderTask>`


Starts rendering the given *task*. This is a blocking call only in background mode.
A blocking render means that this function returns only when the render finishes (from failure or success). 

This function should only be used to render with a Write node or DiskCache node.


.. method:: NatronEngine.App.render(tasks)


    :param tasks: :class:`sequence` 

This is an overloaded function. Same as :func:`render(task)<NatronEngine.App.render>`
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
inform the user of various informations, warnings or errors. 


.. method:: NatronEngine.App.saveProject(filename)

	:param filename: :class:`str<NatronEngine.std::string>`
	:rtype: :class:`bool<PySide.QtCore.bool`
	
	Saves the current project under the current project name. Otherwise if the project has
	never been saved so far, this function asks the user where to save the project in GUI
	mode and in background mode saves the project to the file indicated by the *filename*
	parameter. In GUI mode, *filename* is disregarded.
	
	This function returns *True* if it saved successfully, *False* otherwise.

.. method:: NatronEngine.App.saveProjectAs(filename)

	:param filename: :class:`str<NatronEngine.std::string>`
	:rtype: :class:`bool<PySide.QtCore.bool`
	
	In GUI mode, prompts the user to save the project at some location. In background mode,
	the project is saved to *filename*.
	
	This function returns *True* if it saved successfully, *False* otherwise.
	
	

.. method:: NatronEngine.App.saveTempProject(filename)

	:param filename: :class:`str<NatronEngine.std::string>`
	:rtype: :class:`bool<PySide.QtCore.bool
	
	Saves a copy of the project to the given *filename* without updating project properties
	such as the project path, last save time etc...
	This function returns *True* if it saved successfully, *False* otherwise.

	
.. method:: NatronEngine.App.loadProject(filename)

	:param filename: :class:`str<NatronEngine.std::string>`
	:rtype: :class:`App<NatronEngine.App>`
	
	Loads the project indicated by *filename* in a new :ref:`App<NatronEngine.App>`.
	In GUI mode, this will open a new window only if the current window has modifications.
	This function returns the :ref:`App<NatronEngine.App>` object upon success, *None* otherwise.
	
	
.. method:: NatronEngine.App.resetProject()

	:rtype: :class:`bool<PySide.QtCore.bool`
	
	Attempts to close the current project, without wiping the window. 
	In GUI mode, the user is first prompted to saved his/her changes and can abort the
	reset, in which case this function will return *False*.
	In background mode this function always succeeds, hence always returns *True*.
	this always succeed.
	
.. method:: NatronEngine.App.closeProject()

	:rtype: :class:`bool<PySide.QtCore.bool`
	
	Same as :func:`resetProject()<NatronEngine.App.resetProject>` except that the
	window will close in GUI mode. 
	Also, if this is the last :ref:`App<NatronEngine.App>` alive, Natron will close.
	
.. method:: NatronEngine.App.newProject()

	:rtype: :class:`App<NatronEngine.App>`
	
	Creates a new :ref:`App<NatronEngine.App>`. In GUI mode, this will open a new window.
	Upon success, the :ref:`App<NatronEngine.App>` object is returned, otherwise *None*
	is returned.
	