.. _natronExec:

Natron in command-line
======================

Natron has 3 different execution modes:

- The execution of Natron projects (.ntp)
- The execution of Python scripts that contain commands for Natron
- An interpreter mode where commands can be given directly to the Python interpreter

General options:
----------------

**[--background]** or **[-b]** enables background mode rendering.
 No graphical interface will be shown.
 When using *NatronRenderer* or the *-t* option this argument is implicit and you don't need to use it.
 If using Natron and this option is not specified then it will load the project as if opened from the file menu.


**[--interpreter]** or **[-t]** [optional] *<python script file path>* enables Python interpreter mode.
Python commands can be given to the interpreter and executed on the fly.
An optional Python script filename can be specified to source a script before the interpreter is made accessible.
Note that Natron will not start rendering any Write node of the sourced script, you must explicitly start it.
*NatronRenderer* and *Natron* will do the same thing in this mode, only the *init.py* script will be loaded.


Options for the execution of Natron projects:
---------------------------------------------

::

    Natron <project file path>

**``--writer``** or **``-w``** *<Writer node script name>* [optional] *<filename>* [optional] *<frameRange>* specifies a Write node to render.
When in background mode, the renderer will only try to render with the node script name following this argument.
If no such node exists in the project file, the process will abort.
Note that if you don't pass the *--writer* argument, it will try to start rendering with all the writers in the project.

After the writer node script name you can pass an optional output filename and pass an optional frame range in the format  firstFrame-lastFrame (e.g. 10-40).


.. warning::

    You may only specify absolute file paths with the *-i* option, things like::

        NatronRenderer -i MyReader ~/pictures.png -w MyWriter rendered###.exr

    would not work. This would work on the other hand::

        NatronRenderer -i MyReader /Users/me/Images/pictures.png -w MyWriter /Users/me/Images/rendered###.exr


Note that several *``-w``* options can be set to specify multiple Write nodes to render.

.. warning::

    Note that if specified, then the frame range will be the same for all Write nodes that will render.

**``--reader``* or **``-i``** <reader node script name> <filename> :
Specify the input file/sequence/video to load for the given Reader node.
If the specified reader node cannot be found, the process will abort.

.. warning::

    You may only specify absolute file paths with the *-i* option, things like::

        NatronRenderer -i MyReader ~/pictures.png -w MyWriter rendered###.exr

    would not work. This would work on the other hand::

        NatronRenderer -i MyReader /Users/me/Images/pictures.png -w MyWriter /Users/me/Images/rendered###.exr



**``--onload``** or **``-l``** *<python script file path>* specifies a Python script to be executed
after a project is created or loaded.
Note that this will be executed in GUI mode or with NatronRenderer and it will be executed after any Python function
set to the callback onProjectLoaded or onProjectCreated.
The same rules apply to this script as the rules below on the execution of Python scripts.

**``--render-stats``** or **``-s``** Enables render statistics that will be produced for each frame in form of a file located
next to the image produced by the Writer node, with the same name and a ``-stats.txt`` extension.
The breakdown contains information about each nodes, render times, etc.
This option is useful for debugging purposes or to control that a render is working correctly.
**Please note** that it does not work when writing video files.

Some examples of usage of the tool::

    Natron /Users/Me/MyNatronProjects/MyProject.ntp

    Natron -b -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp

    NatronRenderer -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp

    NatronRenderer -w MyWriter /FastDisk/Pictures/sequence###.exr 1-100 /Users/Me/MyNatronProjects/MyProject.ntp

    NatronRenderer -w MyWriter -w MySecondWriter 1-10 /Users/Me/MyNatronProjects/MyProject.ntp

    NatronRenderer -w MyWriter 1-10 -l /Users/Me/Scripts/onProjectLoaded.py /Users/Me/MyNatronProjects/MyProject.ntp


Example of a script passed to --onload::

    import NatronEngine

    #Create a writer when loading/creating a project
    writer = app.createNode("fr.inria.openfx.WriteOIIO")


Options for the execution of Python scripts:
---------------------------------------------

::

    Natron <Python script path>

Note that the following does not apply if the *-t* option was given.

The script argument can either be the script of a Group that was exported from the graphical user interface or
an exported project or even a script written by hand.

When executing a script, Natron first looks for a function with the following signature::

    def createInstance(app,group):

If this function is found, the script will be imported as a module and it will be executed.

.. warning::

   Note that when imported, the script will not have access to any external variable declared by Natron
   except the variable passed to the createInstance function.

If this function is not found the whole content of the script will be interpreted as though it were given to Python natively.

.. note::

   In this case the script **can** have access to the external variables declared by Natron.

Either cases, the \"app\" variable will always be defined and pointing to the correct application instance.
Note that if you are using Natron in GUI mode, it will source the script before creating the graphical user interface and will not start rendering.
When in command-line mode (*-b* option or NatronRenderer) you must specify the nodes to render.
If nothing is specified, all Write nodes that were created in the Python script will be rendered.

You can render specific Write nodes either with the *-w* option as described above or with the following option:

**[--output]** or **[-o]** *<filename>* *<frameRange>* specifies an *Output* node in the script that should be replaced with a *Write* node.

The option looks for a node named *Output1* in the script and will replace it by a *Write* node
much like when creating a Write node in the user interface.

A filename must be specified, it is the filename of the output files to render.
Also a frame range must be specified if it was not specified earlier.

This option can also be used to render out multiple Output nodes, in which case it has to be used like this:

**[--output1]** or **[-o1]** looks for a node named *Output1*
**[--output2]** or **[-o2]** looks for a node named *Output2*

etc...

**-c** or **[ --cmd ]** "PythonCommand" :
Execute custom Python code passed as a script prior to executing the Python script passed in parameter.
This option may be used multiple times and each python command will be executed in the order they were given to the command-line.


Some examples of usage of the tool::

    Natron /Users/Me/MyNatronScripts/MyScript.py

    Natron -b -w MyWriter /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -w MyWriter /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -o /FastDisk/Pictures/sequence###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -o1 /FastDisk/Pictures/sequence###.exr -o2 /FastDisk/Pictures/test###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -w MyWriter -o /FastDisk/Pictures/sequence###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -w MyWriter /FastDisk/Pictures/sequence.mov 1-100 /Users/Me/MyNatronScripts/MyScript.py -e "print \"Now executing MyScript.py...\""



Options for the execution of the interpreter mode:
---------------------------------------------------

::

    Natron -t [optional] <Python script path>

Natron will first source the script passed in argument, if any and then return control to the user.
In this mode, the user can freely input Python commands that will be interpreted by the Python interpreter shipped with Natron.

Some examples of usage of the tool::

    Natron -t

    NatronRenderer -t

    NatronRenderer -t /Users/Me/MyNatronScripts/MyScript.py



Example
=======

A typical example would be to convert an input image sequence to another format. There are
multiple ways to do it from the command-line in Natron and we are going to show them all:

- Passing a .ntp file to the command line and passing the correct arguments
- Passing a Python script file to the command-line to setup the graph and render

With a Natron project (.ntp) file
----------------------------------


With a Python script file
--------------------------

We would write a customized Python script that we pass to the command-line::

    #This is the content of myStartupScript.py

    reader = app.createReader("/Users/Toto/Sequences/Sequence__####.exr")
    writer = app.createWriter("/Users/Toto/Sequences/Sequence.mov")

    #The node will be accessible via app.MyWriter after this call
    #We do this so that we can reference it from the command-line arguments
    writer.setScriptName("MyWriter")

    #The node will be accessible via app.MyReader after this call
    reader.setScriptName("MyReader")


    #Set the format type parameter of the Write node to Input Stream Format so that the video
    #is written to the size of the input images and not to the size of the project
    formatType =  writer.getParam("formatType")
    formatType.setValue(0)

    #Connect the Writer to the Reader
    writer.connectInput(0,reader)

    #When using Natron (Gui) then the render must explicitly be requested.
    #Otherwise if using NatronRenderer or Natron -b the render will be automatically started
    #using the command-line arguments

    #To use with Natron (Gui) to start render
    #app.render(writer, 10, 20)


To launch this script in the background, you can do it like this::

    NatronRenderer /path/to/myStartupScript.py -w MyWriter 10-20

For now the output filename and the input sequence are *static* and would need to be changed
by hand to execute this script on another sequence.

We can customize the Reader filename and Writer filename parameters using the command-line
arguments::

    NatronRenderer /path/to/myStartupScript.py -i MyReader /Users/Toto/Sequences/AnotherSequence__####.exr -w MyWriter /Users/Toto/Sequences/mySequence.mov 10-20

Let's imagine that now we would need to also set the frame-rate of the video in output and
we would need it to vary for each different sequence we are going to transcode.
This is for the sake of this example, you could also need to modify other parameters in
a real use-case.

Since the fps cannot be specified from the command-line arguments, we could do it in Python with::

    MyWriter.getParam("fps").set(48)

And change the value in the Python script for each call to the command-line, but that would
require manual intervention.

That's where another option from the command-line comes into play: the **``-c``** option
(or ``--cmd``): It allows to pass custom Python code in form of a string that will be
executed before the actual script.

To set the fps from the command-line we could do as such now::

    NatronRenderer /path/to/myStartupScript.py -c "fpsValue=60" -w MyWriter 10-20

Which would require the following modifications to the Python script::

    MyWriter.getParam("fps").set(fpsValue)

We could also set the same way the Reader and Writer file names::

    NatronRenderer /path/to/myStartupScript.py -c "fpsValue=60; readFileName=\"/Users/Toto/Sequences/AnotherSequence__####.exr\"; writeFileName=\"/Users/Toto/Sequences/mySequence.mov\""

And modify the Python script to take into account the new *readFileName* and *writeFileName* parameters::


    ...
    reader = app.createReader(readFileName)
    writer = app.createNode(writeFileName)
    ...


The **``-c``** option can be given multiple times to the command-line and each command passed will
be executed once, in the order they were given.

With a Natron project file:
---------------------------

Let's suppose the user already setup the project via the GUI as such:

MyReader--->MyWriter

We can then launch the render from the command-line this way::

    NatronRenderer /path/to/myProject.ntp -w MyWriter 10-20

We can customize the Reader filename and Writer filename parameters using the command-line
arguments::

    NatronRenderer  /path/to/myProject.ntp -i MyReader /Users/Toto/Sequences/AnotherSequence__####.exr -w MyWriter /Users/Toto/Sequences/mySequence.mov 10-20
