.. _natronPath:

Natron plug-in paths
=====================

When looking for startup scripts or Python group plug-ins, Natron will look into
the following search paths in order:

	* The bundled plug-ins path. This is a directory relative to your Natron installation.
	Usually this is the directory "Plugins" one level up where the executable of Natron is.

	* The standard location for Natron non-OpenFX plug-ins. This is platform dependant.
		On MacOSX that would return::
	
			/Users/<username>/Library/Application Support/INRIA/Natron/Plugins
		
		On Windows::
	
			C:\Users\<username>\AppData\Local\INRIA\Natron\Plugins
		
		On Linux::
	
			/home/<username>/.local/share/INRIA/Natron/Plugins
		
	
	* All the paths indicated by the **NATRON_PLUGIN_PATH** environment variable. This 
	environment variable should contain the separator *;* between each path, such as::
	
		/home/<username>/NatronPluginsA;/home/<username>/NatronPluginsB
	
	* The user extra search paths in the Plug-ins tab of the Preferences of Natron.
	
If the setting "Prefer bundled plug-ins over system-wide plug-ins" is checked in the preferences
then Natron will first look into the bundled plug-ins before checking the standard location.
Otherwise, Natron will check bundled plug-ins as the *last* location. 

Note that if the "User bundled plug-ins" setting in the preferences is unchecked, Natron
will not attempt to load any bundled plug-ins.
