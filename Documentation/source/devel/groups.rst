.. _groups:

Working with groups
===================

Groups in Natron are a complete sub-nodegraph into which the user can manage nodes exactly
like in the *main* nodegraph, but everything in that sub-group will be referenced as 1
node in the hierarchy above, e.g.:

.. figure:: subGroups.png
    :width: 800px
    :align: center


A group can be created like any other node in Natron and by default embeds already 2 nodes:
The **Output** node and one **Input** node.

The **Output** node is used to reference what would be the output of the internal graph of
the group.
In Natron, **a node has necessarily a single output**, hence if you add several *Output* nodes
to a group, **only the first Output node will be taken into account**.

Note that you can also add *Output* nodes to the top-level graph of Natron (the main Node Graph).
They are useful if you need to export your project as a group.

When used in the top-level graph, there can be multiple *Output* nodes, which can then be
used when launching Natron from the command-line to render the script, e.g.:

    NatronRenderer -o1 /FastDisk/Pictures/sequence###.exr -o2 /FastDisk/Pictures/test###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py

Where each argument *o1*, *o2* expand respectively the nodes *Output1* and *Output2*.

.. warning::

        You should never attempt to change the script name of output nodes, otherwise Natron has
        no way to match the given command line arguments to the output nodes. In fact Natron
        will completely ignore your request if you explicitly try to set the script name of an *Output* node.


The **Input** node is not necessarily unique and represents 1 input arrow of the group node.
You can also specify in the settings panel of the *Input* node whether this input should be
considered as a mask or whether it should be optional.

.. note::
    Note that the OpenFX standard specifies that Mask inputs must be optionals so when checking
    the mask parameter, this will automatically check the *optional* parameter.

You can freely rename an **Input** node, effectively changing the label attached to the arrow
on the group node.

.. figure:: inputLabels.png
    :width: 500px
    :align: center

Parameters expressions and groups
---------------------------------

A common task is to add parameters to the group node itself which directly interact to nodes
parameters used internally by this group.

You can add a new parameter to the group node by clicking the "Settings and presets" button
and clicking "Manage user parameters...":

.. figure:: manageUserParams.png
    :width: 300px
    :align: center

A dialog will popup on which you can manage all the parameters that you added. By default
a page is added automatically that will contain user parameters.

.. figure:: addUserParams.png
    :width: 300px
    :align: center

To create a new parameter, click the add button, this brings up a new dialog:

.. figure:: addNewParamDialog.png
    :width: 500px
    :align: center

In this dialog you can configure all the properties of the parameter exactly like you would do using
the :ref:`Python API<Param>`.

Once created, the new parameter can be found in the "User" page of the settings panel:

.. figure:: userPage.png
    :width: 400px
    :align: center

We can then set for instance an expression on the internal blur size parameter to copy the value
of the blur size parameter we just added to the group node:

.. figure:: blurExpression.png
    :width: 500px
    :align: center

The expression is now visible in a green-ish color on the parameter in the settings panel
and the node on the node-graph has a green "E" indicator.

.. figure:: settingsPanelExpression.png
    :width: 400px
    :align: center

.. figure:: exprIndicator.png
    :width: 200px
    :align: center


Exporting a group
------------------

Once your group is setup correctly, you can *export* it as a Python script that Natron will
generate automatically. We call them *PyPlugs*.

To do so, click the **Export as Python plug-in** button in the "Node" page of the settings panel
of the Group node.

.. figure:: exportButton.png
    :width: 400px
    :align: center


Exporting a group as a plug-in, means that it will create a Python script that will be able
to re-create the group entirely and that will be loaded on startup like any other plug-in.
That means that the group will also appear in the left toolbar of Natron and can potentially
have an icon too.


.. figure:: exportWindow.png
    :width: 400px
    :align: center

The *Label* is the name of the plug-in as it will appear in the user interface. It should not
contain spaces or non Python friendly characters as it is going to be used as variable names
in several places.

The *Grouping* is the tool-button under which the plug-in should appear. It accepts
sub-menus notation like this: "Inria/StereoGroups"


The *Icon relative path* is the filepath to an image which should be used as icon for the plug-in.
Note that it is a relative path to the location of the python script.

The *directory* is the location where the script should be written to.
For the plug-in to be loaded by Natron, it should be in its :ref:`search-paths<natronPath>`
hence if you select a directory that is not yet in the search-paths, it will prompt you
to add it.


.. note::

    A re-launch of Natron is required to re-scan the plug-ins and build the tool menus

Once restarted, the plug-in should now appear in the user interface

.. figure:: toolbuttonGroup.png
    :width: 200px
    :align: center

and even in the tab menu of the node-graph:

.. figure:: tabMenuGroup.png
    :width: 200px
    :align: center


.. note::

    The plug-in ID of the group will be exactly the same as the *Label* you picked when
    exporting it, hence when creating a node using the group from a Python script, you
    would do so:

        app.createNode("MyBlurGroup")

    If several plug-ins have the same *pluginID*, Natron will then sort plug-ins by version.

The version of a plug-in by default when exporting it via Natron is 1.

.. warning::

    If 2 plug-ins happen to have the same pluginID and version, Natron will then load the first one
    found in the search paths.

To change the **pluginID** and **version** of your group plug-in, you must implement the 2
following functions in the python script of the group::

    # This function should return an int specifying the version of the plug-in
    # If not implemented, Natron will use 1 by default
    def getVersion():
        return VERSION

    # This function should return a string specifying the ID of the plug-in, for example
    # "fr.inria.groups.customBlur"
    # If not implemented, Natron will use the label as a pluginID
    def getPluginID():
        return UNIQUE_ID


Exporting a project as group
----------------------------

Similarly, Natron allows you to export the top-level node-graph as a Python group plug-in.
From the "File" menu, select "Export project as group".

.. warning::

    To be exportable, your project should at least contain 1 output node.

.. note::

    While this functionality is made for convenience, you should be cautious, as
    exporting a project containing Readers will probably not work very well in another project
    or computer because of file-paths no longer pointing to a valid location.


.. warning::

    If you were to write a group plug-in and then want to have your expressions persist when
    your group will be instantiated, it is important to prefix the name of the nodes you reference
    in your expression by the **thisGroup.** prefix. Without it, Natron thinks you're referencing
    a top-level node, i.e: a node which belongs to the main node-graph, however, since you're using
    a group, all your nodes are no longer top-level and the expression will fail.

Moving nodes between groups
----------------------------

You can create a group from the selection in Natron by holding CTRL+SHIFT+G.
This will effectively move all nodes selected into a new sub-group

You can also copy/cut/paste in-between groups and projects.

Creating a group by hand
------------------------

You can also write a group plug-in by hand using the :ref:`Python API<apiReference>` of Natron.

Natron detects a Python file within the plug-in path as a PyPlug if it contains the following line [1]_::

    # Natron PyPlug

There may be Python files which are neither PyPlugs or Toolsets within these directories, for example python modules.

To work as a plug-in, your script should implemented the following functions::

    # This function is mandatory and should return the label of the plug-in as
    # visible on the user interface
    def getLabel():
        return LABEL

    # This function should return an int specifying the version of the plug-in
    # If not implemented, Natron will use 1 by default
    def getVersion():
        return VERSION

    # This function should return a string specifying the ID of the plug-in, for example
    # "fr.inria.groups.customBlur"
    # If not implemented, Natron will use the label as a pluginID
    def getPluginID():
        return UNIQUE_ID

    # This function should return a string specifying the relative file path of an image
    # file relative to the location of this Python script.
    # This function is optional.
    def getIconPath():
        return ICON_PATH

    # This function is mandatory and should return the plug-in grouping, e.g.:
    # "Other/Groups"
    def getGrouping():
        return GROUPING

    # This function is optional and should return a string describing the plug-in to the user.
    # This is the text that will show up when the user press the "?" button on the settings panel.
    def getDescription():
        return DESCRIPTION

    # This function is mandatory and should re-create all the nodes and parameters state
    # of the group.
    # The group parameter is a group node that has been created by Natron and that  will host all
    # the internal nodes created by this function.
    # The app parameter is for convenience to have access in a generic way to the app object,
    # no matter in which project instance your script is invoked in.
    def createInstance(app, group):
        ...

The Python group plug-ins generated automatically by Natron are a good start to figure out
how to write scripts yourself.

.. warning::

    Python group plug-ins should avoid using any functionality provided by the :ref:`NatronGui<NatronGui>` module
    because it would then break their compatibility when working in command-line background mode.
    The reason behind this is that the Python module NatronGui is not imported in command-line mode because
    internally it relies on the QtGui library, which may not be present on some render-farms.
    Attempts to load PyPlugs relaying on the NatronGui module would then fail and the rendering
    would abort.


.. warning::

    Note that PyPlugs are **imported** by Natron which means that the script will not have access
    to any external variable declared by Natron except the variables passed to the createInstance function
    or the attributes of the modules imported.

.. _pyPlugHandWritten:

Adding hand-written code (callbacks, etc...)
--------------------------------------------

It is common to add hand-written code to a PyPlug. When making changes to the PyPlug from
the GUI of Natron, exporting it again will overwrite any change made to the python script
of the PyPlug.
In order to help development, all hand-written code can be written in a separate script
with the **same** name of the original Python script but ending with *Ext.py*, e.g.:

    MyPyPlugExt.py

This extension script can contain for example the definition of all callbacks used in the PyPlug.
When calling the *createInstance(app,group)* function, the PyPlug will call right at the end
of the function the *createInstanceExt(app,group)* function. You can define it in your
*extension script* if you want to apply extra steps to the creation of the group. For example
you might want to actually set the callbacks on the group::

    #This is in MyPyPlugExt.py

    def paramChangedCallback(thisParam, thisNode, thisGroup, app, userEdited):
        print thisParam.getScriptName()

    def createInstanceExt(app,group):
        # Note that the callback belongs to the PyPlug to so we use it as prefix
        group.onParamChanged.set("MyPyPlug.paramChangedCallback")

.. note::

    Note that callbacks don't have to be registered with the extension module prefix but just
    with the PyPlug's name prefix since the *"from ... import *"* statement is made to import
    the extensions script.

Starting Natron with a script in command line
----------------------------------------------

Natron can be started with a Python script as argument.

When used in background mode (i.e: using NatronRenderer or Natron with the option **-b**)
Natron will do the following steps:

- Source the script
- If found, run a function with the following signature *createInstance(app,group)*
- Start rendering the specified writer nodes (with the **-w** option) and/or the *Output* nodes (with the **-o** option)

This allows to pass a group plug-in to Natron and render it easily if needed.
Also, it can take arbitrary scripts which are not necessarily group plug-ins.

When Natron is launched in GUI mode but with a Python script in argument, it will do the following steps:

- Source the script
- If found, run a function with the following signature *createInstance(app,group)*


Toolsets
--------

Toolsets in Natron are a predefined set of actions that will be applied to the node-graph.
They work exactly like PyPlugs except that no actual group node will be created, only
the content of the *createInstance(app,group)* function will be executed.

This useful to create pre-defined graphs, for example like the Split and Join plug-in
in the Views menu.

To be recognized as a toolset, your PyPlug must implement the following function::

    def getIsToolset():
        return True

Also the **group** parameter passed to the *createInstance(app,group)* function
will be *None* because no group node is actually involved.

As with regular PyPlugs, the file must also contrain the line [1]_::

    # Natron PyPlug

.. [1] There was a bug in Natron versions 2.1.0 through 2.3.14 which prevented loading PyPlugs and Toolsets if they did not have a line that started with::

        # This file was automatically generated by Natron PyPlug exporter
