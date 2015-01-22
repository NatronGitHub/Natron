.. _introduction:

Start-up scripts
================

On start-up Natron will run different start-up scripts to let you setup anything like callbacks,
menus, etc... 

There are 2 different initialization scripts that Natron will look for in the :ref:`search paths<natronPath>`.

	* **init.py**
	
		This script is always run and should only initialize non-GUI stuff. You may not use
		it to initialize e.g new menus or windows. Generally this is a good place to initialize
		all the callbacks that you may want to use in your projects.
		
	* **initGui.py**
	
		This script is only run in GUI mode (that is with the user interface). It should 
		initialize all gui-specific stuff like new menus or windows.
	
All the scripts with the above name found in the search paths will be run in the order
of the :ref:`search paths<natronPath>`.