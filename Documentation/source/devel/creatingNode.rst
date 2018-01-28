.. _creatingNode:

Creating and controlling nodes
===============================


Creating a new node:
--------------------

To create a :doc:`node<PythonReference/NatronEngine/Effect>` in Natron, you would do so
using the :doc:`app instance<PythonReference/NatronEngine/Effect>` via the function
:func:`createNode(pluginId,majorVersion,group)<NatronEngine.App.createNode>` like this::

    app1.createNode("fr.inria.openfx.ReadOIIO")

In this line we specify that we want the first opened project to create a node instantiating
the plug-in *ReadOIIO*.
Note that if we were in background mode we could just write the following which would be equivalent::

    app.createNode("fr.inria.openfx.ReadOIIO")

Since in command-line there is only a single project opened, Natron does the following assignment::

    app = app1

If we were to create the node into a specific group, we would do so like this::

    group = app.createNode("fr.inria.built-in.Group")

    reader = app.createNode("fr.inria.openfx.ReadOIIO", -1, group)

Note that when passed the number -1, it specifies that we want to load the highest version
of the plug-in found. This version parameter can be useful to load for example a specific
version of a plug-in.

The *pluginID* passed to this function is a **unique** ID for each plug-in. If 2 plug-ins
were to have the same ID, then Natron will create separate entries for each version.

You can query all plug-ins available in Natron this way::

    allPlugins = natron.getPluginIDs()

You can also filter out plug-ins that contain only a given *filter* name::

    # Returns only plugin IDs containing ".inria" in it

    filteredPlugins = natron.getPluginIDs(".inria.")

In the user interface, the plug-in ID can be found when pressing the **?** button located in the
top right-hand corner of the settings panel:

.. figure:: helpButton.png
    :width: 300px
    :align: center

.. figure:: pluginID.png
    :width: 400px
    :align: center

Connecting a node to other nodes:
-----------------------------------

To connect a node to the input of another node you can use the :func:`connectInput(inputNumber,input)<NatronEngine.Effect.connectInput>` function.

The *inputNumber* is a 0-based index specifying the input on which the function should connect the given *input* :doc:`Effect<PythonReference/NatronEngine/Effect>`.

You can query the input name at a specific index with the following function::

    print(node.getInputLabel(i))

Here is a small example where we would create 3 nodes and connect them together::

    #Create a write node
    writer = app.createNode("fr.inria.openfx.WriteOIIO")

    #Create a blur
    blur = app.createNode("net.sf.cimg.CImgBlur")

    #Create a read node
    reader = app.createNode("fr.inria.openfx.ReadOIIO")

    #Connect the write node to the blur
    writer.connectInput(0,blur)

    #Connect the blur to the read node
    blur.connectInput(0,reader)

Note that the following script would do the same since nodes are :ref:`auto-declared variables<autoVar>`
::

    node = app.createNode("fr.inria.openfx.WriteOIIO")
    print(node.getScriptName()) # prints WriteOIIO1

    #The write node is now available via its script name app.WriteOIIO1

    node = app.createNode("net.sf.cimg.CImgBlur")
    print(node.getScriptName()) # prints CImgBlur1

    #The blur node is now available via its script name app.BlurCImg1

    node = app.createNode("fr.inria.openfx.ReadOIIO")
    print(node.getScriptName()) # prints ReadOIIO1

    #The ReadOIIO node is now available via its script name app.ReadOIIO1

    app.WriteOIIO1.connectInput(0,app.BlurCImg1)
    app.BlurCImg1.connectInput(0,app.ReadOIIO1)


Note that not all connections are possible, and sometimes it may fail for some reasons explained
in the documentation of the :func:`connectInput(inputNumber,input)<NatronEngine.Effect.connectInput>` function.

You should then check for errors this way::

    if not app.WriteOIIO1.connectInput(0,app.BlurCImg1):
        # Handle errors

You can check beforehand whether a subsequent *connectInput* call would succeed or not
by calling the :func:`canConnectInput(inputNumber,input)<NatronEngine.Effect.connectInput>` which basically
checks whether is is okay to do the connection or not. You can then safely write the following instructions::

    if app.WriteOIIO1.canConnectInput(0,app.BlurCImg1):
        app.WriteOIIO1.connectInput(0,app.BlurCImg1)
    else:
        # Handle errors

Note that internally *connectInput* calls *canConnectInput* to validate whether the connection is possible.

To disconnect an existing connection, you can use the :func:`disconnectInput(inputNumber)<NatronEngine.Effect.disconnectInput>` function.
