.. _pysideExample:

PySide panels
=============

To create a non-modal :ref:`panel<pypanel>` that can be saved in the project's layout and 
docked into the application's :ref:`tab-widgets<pyTabWidget>`, there is 2 possible way of 
doing it:

	* Sub-class :ref:`PyPanel<pypanel>` and create your own GUI using `PySide <http://qt-project.org/wiki/PySideDocumentation>`_ 
	
	
	* Use the API proposed by :ref:`PyPanel<pypanel>` to add custom user :ref:`parameters<Param>` as done for :ref:`PyModalDialog<pyModalDialog>`.
	
	
Generally you should define your panels in the **initGui.py** script (see :ref:`startup-scripts<startupScripts>`).
You can also define the panel in the *Script Editor* at run-time of Natron, though this will not persist
when Natron is closed.

To make your panel be created upon new project created, register a Python callback in the *Preferences-->Python* tab
in the parameter *After project created*.
This callback will not be called for project being loaded either via an auto-save or via a user action.
::

	#This goes in initGui.py
	
	def createMyPanel():
		#Create panel
		...

	def onProjectCreatedCallback():
		createMyPanel()
		
.. warning::

	When the initGui.py script is executed, the *app* variable (or any derivative such as *app1* *app2* etc...)
	does not exist since no project is instantiated yet. The purpose of the script is not to instantiate the GUI per-say
	but to define classes and functions that will be used later on by :ref:`application instances<GuiApp>`.	
	
Python panels can be re-created for existing projects using serialization
functionalities explained :ref:`here<panelSerialization>`.

The sole requirement to save a panel in the layout is to call the :func:`registerPythonPanel(panel,function)<NatronGui.GuiApp.registerPythonPanel>` function
of :ref:`GuiApp<GuiApp>` .

Using user parameters:
----------------------

Let's assume we have no use to make our own widgets and want quick parameters fresh and ready,
we just have to use the :ref:`PyPanel<pypanel>` class without sub-classing it.



