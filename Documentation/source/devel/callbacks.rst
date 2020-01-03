.. _callbacks:

Using Callbacks
===============

*Callbacks* are functions that are executed after or before a certain event in Natron.
They are Python-defined methods that you declare yourself and then register to Natron
in a different manner for each callback.

This document describes the signature that your different callbacks must have in order
to work for each event. The parameters of your declaration must match exactly the same
signature otherwise the function call will not work.

.. warning::

    Note that callbacks will be called in background and GUI modes, hence you should
    wrap all GUI code by the following condition::

        if not NatronEngine.natron.isBackground():
            #...do gui stuff

Callback persistence
--------------------

If you want your callback to persist 2 runs of Natron; it is necessary that you define it
in a script that is loaded by Natron, that is, either the **init.py** script (or **initGui.py** if you want it only available in GUI mode)
or the script of a Python group plug-in (or its extension script, see :ref:`here<pyPlugHandWritten>`).
See :ref:`this section<startupScripts>` for more infos.

Here is the list of the different callbacks:

The param changed callback
--------------------------

This function is called every times the value of a :ref:`parameter<Param>` changes.
This callback is available for all objects that can hold parameters,namely:

- :ref:`Effect<Effect>`
- :ref:`PyPanel<pypanel>`
- :ref:`PyModalDialog<pyModalDialog>`


The signature of the callback used on the :ref:`Effect<Effect>` is::

    callback(thisParam, thisNode, thisGroup, app, userEdited)

- **thisParam** : This is a :ref:`Param<Param>` pointing to the parameter which just had its value changed.
- **thisNode** : This is a :ref:`Effect<Effect>` pointing to the effect holding **thisParam**
- **thisGroup** : This is a :ref:`Effect<Effect>` pointing to the group  holding **thisNode** or **app** otherwise if the node is in the main node-graph.
- **app** : This variable will be set so it points to the correct :ref:`application instance<App>`.
- **userEdited** : This indicates whether or not the parameter change is due to user interaction (i.e: because the user changed
  the value by theirself) or due to another parameter changing the value of the parameter
  via a derivative of the :func:`setValue(value)<>` function.

For the *parameter changed callback* of :ref:`PyPanel<pypanel>` and :ref:`PyModalDialog<pyModalDialog>`, the signature of the callback function is:

    callback(paramName, app, userEdited)

- **paramName** indicating the :ref:`script-name<autoVar>` of the parameter which just had its value changed.
- **app** : This variable will be set so it points to the correct :ref:`application instance<App>`.
- **userEdited** : This indicates whether or not the parameter change is due to user interaction (i.e: because the user changed the value by theirself) or due to another parameter changing the value of the parameter via a derivative of the :func:`setValue(value)<>` function.

.. note::

    The difference between the callbacks on  :ref:`PyPanel<pypanel>` and :ref:`PyModalDialog<pyModalDialog>` and
    :ref:`Effect<Effect>` is due to technical reasons: mainly because the parameters of the
    :ref:`PyPanel<pypanel>` class and :ref:`PyModalDialog<pyModalDialog>` are not declared
    as attributes of the object.


Registering the param changed callback
----------------------------------------

To register the param changed callback of an :ref:`Effect<Effect>`, you can do so in
the settings panel of the node, in the "Node" tab, by entering the name of your Python function:

.. figure:: settingsPanelParamChangedCB.png
    :width: 400px
    :align: center

You can also set the callback directly from the script: The callback is just another :ref:`parameter<Param>`
of the node, on which you can call :func:`setValue(value)<>` to set the name of the callback

::

    def myBlurCallback(thisParam, thisNode, thisGroup, app, userEdited):
        ...

    app.BlurCImg1.onParamChanged.set("myBlurCallback")

.. note::

    If the callback is defined in a separate python file, such as the python script of a
    python group plug-in, then do not forget the module prefix, e.g.:

        app.MyPlugin1.BlurCImg1.onParamChanged.set("MyPlugin.myBlurCallback")

Example
^^^^^^^^
::

    # This simple callback just prints a string when the "size" parameter of the BlurCImg
    # node changes
    def myBlurCallback(thisParam, thisNode, thisGroup, app, userEdited):
        if thisParam == thisNode.size:
            print("The size of the blur just changed!")

    app.BlurCImg1.onParamChanged.set("myBlurCallback")



Using the param changed callback for  :ref:`PyModalDialog<pyModalDialog>` and  :ref:`PyPanel<pypanel>`
--------------------------------------------------------------------------------------------------------------------


To register the callback to the object, use the :func:`setParamChangedCallback(pythonFunctionName)<>` function.

The following example is taken from the initGui.py script provided as example in :ref:`this section<sourcecodeEx>`.

Example
^^^^^^^^

::

    #Callback called when a parameter of the player changes
    #The variable paramName is declared by Natron; indicating the name of the parameter which just had its value changed
    def myPlayerParamChangedCallback(paramName, app, userEdited):

        viewer = app.getViewer("Viewer1")
        if viewer == None:
            return
        if paramName == "previous":
            viewer.seek(viewer.getCurrentFrame() - 1)
        elif paramName == "backward":
            viewer.startBackward()
        elif paramName == "forward":
            viewer.startForward()
        elif paramName == "next":
            viewer.seek(viewer.getCurrentFrame() + 1)
        elif paramName == "stop":
            viewer.pause()


    def createMyPlayer():
        app.player = NatronGui.PyPanel("fr.inria.myplayer","My Player",True,app)
        #...
        app.player.setParamChangedCallback("myPlayerParamChangedCallback")

The After input changed callback
----------------------------------

Similarly to the param changed callback, this function is called whenever an input connection of
the node is changed.  The signature is::

    callback(inputIndex, thisNode, thisGroup, app)

.. note::

    This function will be called even when loading a project

- **inputIndex** : This is the input which just got connected/disconnected.
  You can fetch the input at the given index with the :func:`getInput(index)<>` function of the :ref:`Effect<Effect>` class.

- **thisNode** : This is a :ref:`Effect<Effect>` holding the input which just changed

- **thisGroup** : This is a :ref:`Effect<Effect>` pointing to the group  holding **thisNode**. Note that it will be declared only if **thisNode** is part of a group.

- **app** : points to the correct :ref:`application instance<App>`.

Registering the input changed callback
----------------------------------------

To register the input changed callback of an :ref:`Effect<Effect>`, you can do so in
the settings panel of the node, in the "Node" tab, by entering the name of your Python function:

.. figure:: inputChangedPanel.png
    :width: 400px
    :align: center

You can also set the callback directly from the script: The callback is just another :ref:`parameter<Param>`
of the node, on which you can call :func:`setValue(value)<>` to set the name of the callback

::

    def inputChangedCallback(inputIndex, thisNode, thisGroup, app):
        ...

    app.Merge1.onInputChanged.set("inputChangedCallback")


Example
^^^^^^^^
::

    # This simple callback just prints the input node name if connected or "None" otherwise
    # node changes
    def inputChangedCallback(inputIndex, thisNode, thisGroup, app):
        inp = thisNode.getInput(inputIndex)
        if not inp is None:
            print("Input ",inputIndex," is ",inp.getScriptName())
        else:
            print("Input ",inputIndex," is None")

    app.Merge1.onInputChanged.set("inputChangedCallback")


The After project created callback
-------------------------------------

This function is called whenever a new project is created, that is either when launching Natron
without loading a project, or when clicking "Create a new project" or "Close project".

.. note::

    Note that this function is never called when a project is loaded either via an auto-save
    or from user interaction.

The **app** variable will be set so it points to the correct :ref:`application instance<App>`
being created.

You can set the callback via the *afterProjectCreated* parameter of the settings of Natron.

.. figure:: preferencesCallback.png
    :width: 400px
    :align: center

This is a good place to create custom panels and/or setup the node-graph with node presets.

Example, taken from the initGui.py script provided as example in :ref:`this section<sourcecodeEx>`:

::

    def onProjectCreated():

        #Always create our icon viewer on project creation
        createIconViewer()


    natron.settings.afterProjectCreated.set("onProjectCreated")



The After project loaded callback
-------------------------------------

This function is very similar to the After project created callback but is a per-project callback,
called only when a project is loaded from an auto-save or from user interaction.
The signature is::

    callback(app)


- **app** : points to the correct :ref:`application instance<App>` being loaded.

You can set this callback in the project settings:

.. figure:: projectCallbacks.png
    :width: 400px
    :align: center

This is a good place to do some checks to opened projects or to setup something:

::

    def onProjectLoaded(app):

        if not natron.isBackground():
            if app.getUserPanel("fr.inria.iconviewer") is None:
                createIconViewer()

    app.afterProjectLoad.set("onProjectLoaded")

.. note::

    You can set a default After project loaded callback for all new projects in the *Preferences-->Python* tab.

The Before project save callback
----------------------------------

This function will be called prior to saving a project either via an auto-save or from
user interaction. The signature is::

    callback(filename, app, autoSave)

- **filename** : This is the file-path where the project is initially going to be saved.

- **app** :  points to the correct :ref:`application instance<App>` being created.

- **autoSave** : This indicates whether the save was originated from an auto-save or from user interaction.

.. warning::

        This function should return the filename under which the project should really be saved.

You can set the callback from the project settings:

.. figure:: projectCallbacks.png
    :width: 400px
    :align: center


::

    def beforeProjectSave(filename, app, autoSave):
        print("Saving project under: ",filename)
        return filename

    app.beforeProjectSave.set("beforeProjectSave")

.. note::

    You can set a default Before project save callback for all new projects in the *Preferences-->Python* tab.


The Before project close callback
---------------------------------

This function is called prior to closing a project either because the application is about
to quit or because the user closed the project. The signature is::

    callback(app)

- **app** : points to the correct :ref:`application instance<App>` being closed.

This function can be used to synchronize any other device or piece of software communicating
with Natron.

You can set the callback from the project settings:

.. figure:: projectCallbacks.png
    :width: 400px
    :align: center

::

    def beforeProjectClose(app):
        print("Closing project)

    app.beforeProjectClose.set("beforeProjectClose")

.. note::

    You can set a default Before project close callback for all new projects in the *Preferences-->Python* tab.


The After node created callback
---------------------------------

This function is called after creating a node in Natron. The signature is::

    callback(thisNode, app, userEdited)


- **thisNode** points to the :ref:`node<Effect>` that has been created.

- **app** points to the correct :ref:`application instance<App>`.

- **userEdited** will be *True* if the node was created
  by the user (or by a script using the :func:`createNode(pluginID,version,group)<>` function)
  or *False* if the node was created by actions such as pasting a node or when the project is
  loaded.

This is a good place to change default parameters values.

You can set the callback from the project settings:

.. figure:: projectCallbacks.png
    :width: 400px
    :align: center

::

    def onNodeCreated(thisNode, app, userEdited):
        print(thisNode.getScriptName()," was just created")
        if userEdited:
            print(" due to user interaction")
        else:
            print(" due to project load or node pasting")

    app.afterNodeCreated.set("onNodeCreated")

.. note::

    You can set a default After node created callback for all new projects in the *Preferences-->Python* tab.

This callback can also be set in the *Node* tab of any **Group** node (or *PyPlug*).
If set on the Group, the callback will be invoked for the *Group* node and all its direct children (not recursively).

The Before node removal callback:
---------------------------------

This function is called prior to deleting a node in Natron. The signature is::

    callback(thisNode, app)

- **thisNode** : points to the :ref:`node<Effect>` about to be deleted.

- **app** : points to the correct :ref:`application instance<App>`.


.. warning::

    This function will **NOT** be called when the project is closing

You can set the callback from the project settings:

.. figure:: projectCallbacks.png
    :width: 400px
    :align: center

::

    def beforeNodeDeleted(thisNode, app):
        print(thisNode.getScriptName()," is going to be destroyed")


    app.beforeNodeRemoval.set("beforeNodeDeleted")

.. note::

    You can set a default Before node removal callback for all new projects in the *Preferences-->Python* tab.

This callback can also be set in the *Node* tab of any **Group** node (or *PyPlug*).
If set on the Group, the callback will be invoked for the *Group* node and all its direct children (not recursively).

The Before frame render callback:
---------------------------------

This function is called prior to rendering any frame with a Write node. The signature is::

    callback(frame, thisNode, app)

- **thisNode** : points to the :ref:`write node<Effect>`.

- **app** : points to the correct :ref:`application instance<App>`.

- **frame**: The frame that is about to be rendered

To execute code specific when in background render mode or in GUI mode, use the following condition
::

    if natron.isBackground():
        #We are in background mode

You can set the callback from the Write node settings panel in the "Python" tab.

.. figure:: writePython.png
    :width: 400px
    :align: center

This function can be used to communicate with external programs for example.

.. warning::

    Any exception thrown in this callback will abort the render

The After frame rendered callback:
-----------------------------------

This function is called after each frame is finished rendering with a Write node.
 The signature is::

    callback(frame, thisNode, app)

- **thisNode** : points to the :ref:`write node<Effect>`.

- **app** : points to the correct :ref:`application instance<App>`.

- **frame**: The frame that is about to be rendered

To execute code specific when in background render mode or in GUI mode, use the following condition
::

    if natron.isBackground():
        #We are in background mode

You can set the callback from the Write node settings panel in the "Python" tab.

.. figure:: writePython.png
    :width: 400px
    :align: center

This function can be used to communicate with external programs for example.

.. warning::

    Any exception thrown in this callback will abort the render

The Before render callback:
---------------------------

This function is called once before starting rendering the first frame of a sequence with
the Write node.  The signature is::

    callback(frame, thisNode, app)

- **thisNode** : points to the :ref:`write node<Effect>`.

- **app** : points to the correct :ref:`application instance<App>`.

To execute code specific when in background render mode or in GUI mode, use the following condition
::

    if natron.isBackground():
        #We are in background mode

You can set the callback from the Write node settings panel in the "Python" tab.

.. figure:: writePython.png
    :width: 400px
    :align: center

This function can be used to communicate with external programs for example.

.. warning::

    Any exception thrown in this callback will abort the render

.. _afterRenderCallback:

The After render callback:
---------------------------

This function is called once after the rendering of the last frame is finished with
the Write node or if the render was aborted.  The signature is::

    callback(aborted, thisNode, app)

- **aborted** :  *True* if the rendering was aborted or *False* otherwise.

- **thisNode** : points to the :ref:`write node<Effect>`.

- **app** : points to the correct :ref:`application instance<App>`.


To execute code specific when in background render mode or in GUI mode, use the following condition
::

    if natron.isBackground():
        #We are in background mode

You can set the callback from the Write node settings panel in the "Python" tab.

.. figure:: writePython.png
    :width: 400px
    :align: center

This function can be used to communicate with external programs for example.
