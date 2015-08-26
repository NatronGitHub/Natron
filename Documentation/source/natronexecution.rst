.. _natronExec:

Natron in command-line
======================

Natron has 3 different execution modes:

	* The execution of Natron projects (.ntp)
	* The execution of Python scripts that contain commands for Natron
	* An interpreter mode where commands can be given directly to the Python interpreter

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

**[--writer]** or **[-w]** *<Writer node script name>* [optional] *<filename>* [optional] *<frameRange>* specifies a Write node to render.
When in background mode, the renderer will only try to render with the node script name following this argument.
If no such node exists in the project file, the process will abort.
Note that if you don't pass the *--writer* argument, it will try to start rendering with all the writers in the project.

After the writer node script name you can pass an optional output filename and pass an optional frame range in the format  firstFrame-lastFrame (e.g: 10-40). 

Note that several *-w* options can be set to specify multiple Write nodes to render.

.. warning::

	Note that if specified, then the frame range will be the same for all Write nodes that will render.
	

**[--onload]** or **[-l]** *<python script file path>* specifies a Python script to be executed
after a project is created or loaded.
Note that this will be executed in GUI mode or with NatronRenderer and it will be executed after any Python function
set to the callback onProjectLoaded or onProjectCreated.
The same rules apply to this script as the rules below on the execution of Python scripts.

**[ --render-stats]** or **[-s]** Enables render statistics that will be produced for each frame in form of a file located
 next to the image produced by the Writer node, with the same name and a -stats.txt extension. 
The breakdown contains informations about each nodes, render times etc...
This option is useful for debugging purposes or to control that a render is working correctly.

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
When in command-line mode (*-b* option or NatronRenderer) you must specify the nodes to render either with the *-w* option as described above or with the following option:

**[--output]** or **[-o]** *<filename>* *<frameRange>* specifies an *Output* node in the script that should be replaced with a *Write* node.

The option looks for a node named *Output1* in the script and will replace it by a *Write* node
much like when creating a Write node in the user interface.

A filename must be specified, it is the filename of the output files to render.
Also a frame range must be specified if it was not specified earlier.

This option can also be used to render out multiple Output nodes, in which case it has to be used like this:

**[--output1]** or **[-o1]** looks for a node named *Output1* 
**[--output2]** or **[-o2]** looks for a node named *Output2* 

etc...

Some examples of usage of the tool::

	Natron /Users/Me/MyNatronScripts/MyScript.py
	
	Natron -b -w MyWriter /Users/Me/MyNatronScripts/MyScript.py
	
	NatronRenderer -w MyWriter /Users/Me/MyNatronScripts/MyScript.py
	
	NatronRenderer -o /FastDisk/Pictures/sequence###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py
	
	NatronRenderer -o1 /FastDisk/Pictures/sequence###.exr -o2 /FastDisk/Pictures/test###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py
	
	NatronRenderer -w MyWriter -o /FastDisk/Pictures/sequence###.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py


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
