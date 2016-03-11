.. _natronObjects:

Objects hierarchy Overview
==========================

When running Natron, several important objects are created automatically and interact at 
different levels of the application.

The main object in Natron is the :doc:`PythonReference/NatronEngine/PyCoreApplication` class
which represents the unique instance of the process. It is available directly via the variable::
	
	natron
	
Basically it handles all *application-wide* informations about plug-ins, environment,
 :doc:`application settings<PythonReference/NatronEngine/AppSettings>`...
but also can hold one or multiple :doc:`application instance<PythonReference/NatronEngine/App>` 
which are made available to the global variables via the following variables::

	app1 # References the first instance of the application (the first opened project)
	app2 # The second project
	...
	
Note that in background command-line mode, there would always be a single opened project
so Natron does the following assignment for you::

	app = app1
	
.. warning::

	Note that when running scripts in the *Script Editor*, the application is running in GUI
	mode hence the *app* variable is not declared.


The :doc:`PythonReference/NatronEngine/App` object is responsible for managing all informations
relative to a project. This includes all the :doc:`nodes<PythonReference/NatronEngine/Effect>`, 
project settings and render controls. See :ref:`this section <creatingNode>` to create 
and control nodes.

Each node can have :doc:`parameters<PythonReference/NatronEngine/Param>` which are the controls
found in the settings panel of the node.

The same :doc:`PythonReference/NatronEngine/Param` class is also used for the project settings
and the application settings (preferences). 

