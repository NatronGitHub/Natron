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
functionalities explained :ref:`here<panelSerialization>`
See the example below (the whole script is available attached below)
::

    # We override the save() function and save the filename
    def save(self):
        return self.locationEdit.text()

    # We override the restore(data) function and restore the current image
    def restore(self,data):

        self.locationEdit.setText(data)
        self.label.setPixmap(QPixmap(data))


The sole requirement to save a panel in the layout is to call the :func:`registerPythonPanel(panel,function)<NatronGui.GuiApp.registerPythonPanel>` function
of :ref:`GuiApp<GuiApp>`::

    app.registerPythonPanel(app.mypanel,"createIconViewer")

See the details of the :ref:`PyPanel<pypanel>` class for more explanation on how to
sub-class it.

Also check-out the complete example :ref:`source code<sourcecodeEx>` below.

Using user parameters:
----------------------

Let's assume we have no use to make our own widgets and want quick :ref:`parameters<Param>` fresh and ready,
we just have to use the :ref:`PyPanel<pypanel>` class without sub-classing it::

    #Callback called when a parameter of the player changes
    #The variable paramName is declared by Natron; indicating the name of the parameter which just had its value changed
    def myPlayerParamChangedCallback():

        viewer = app.getViewer("Viewer1")
        if viewer == None:
            return
        if paramName == "previous":
            viewer.seek(viewer.getCurrentFrame() - 1)



    def createMyPlayer():

        #Create a panel named "My Panel" that will use user parameters
        app.player = NatronGui.PyPanel("fr.inria.myplayer","My Player",True,app)

        #Add a push-button parameter named "Previous"
        app.player.previousFrameButton = app.player.createButtonParam("previous","Previous")

        #Refresh user parameters GUI, necessary after changes to static properties of parameters.
        #See the Param class documentation
        app.player.refreshUserParamsGUI()

        #Set a callback that will be called upon parameter change
        app.player.setParamChangedCallback("myPlayerParamChangedCallback")


.. note::

    For convenience, there is a way to also add custom widgets to python panels that are
    using user parameters with the :func:`addWidget(widget)<>` and :func:`insertWidget(index,widget)<>`
    functions. However the widgets will be appended **after** any user parameter defined.


Managing panels and panes
--------------------------

Panels in Natron all have an underlying script-name, that is the one you gave as first parameter
to the constructor of :ref:`PyPanel<pypanel>`.

You can then move the :ref:`PyPanel<pypanel>` between the application's :ref:`panes<pyTabWidget>` by calling
the function :func:`moveTab(scriptName,pane)<>` of :ref:`GuiApp<GuiApp>`.

.. note::

    All application's panes are :ref:`auto-declared<autoVar>` by Natron and can be referenced directly
    by a variable, such as:

        app.pane2

Panels also have a script-name but only :ref:`viewers<pyViewer>` and :ref:`user panels<pypanel>`
are auto-declared by Natron::

    app.pane2.Viewer1
    app.pane1.myPySidePanelScriptName


.. _sourcecodeEx:

Source code of the example initGui.py
---------------------------------------

.. literalinclude:: initGui.py
