.. _natronObjects:

Objects hierarchy Overview
==========================

When running Natron, several important objects are created automatically and interact at
different levels of the application.

Natron is separated in 2 internal modules:

:ref:`NatronEngine<NatronEngine>` and :ref:`NatronGui<NatronGui>`.

The latest is only available in **GUI** mode. You may access *globally* to the Natron
process with either **NatronEngine.natron** or **NatronGui.natron**

NatronEngine.natron is of type :doc:`PyCoreApplication<PythonReference/NatronEngine/PyCoreApplication>` and
NatronGui.natron is of type :doc:`PyGuiApplication<PythonReference/NatronGui/PyGuiApplication>`.
This is a singleton and there is only a **single** instance of that variable living throughout the
execution of the Natron process.

When using with **NatronGui.natron** you get access to GUI functionalities in addition
to the internal functionalities exposed by :doc:`PyCoreApplication<PythonReference/NatronEngine/PyCoreApplication>`

Basically if using Natron in command-line you may only use **NatronEngine.natron**.


.. note::

    You may want to use **natron** directly to avoid prefixing everything with *NatronEngine.*
    or *NatronGui.* by using a from NatronEngine import * statement. Be careful though as
    it then makes it more confusing for people reading the code as to which version of the **natron**
    variable you are using.

It handles all *application-wide* information about plug-ins, environment,
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

    In GUI mode, don't rely on the *app* variable in the *Script Editor*. Natron sets it
    before running a callback, a menu command or a loaded script, so it may exist, but it then
    points to whichever project instance ran that code last. Use *app1*, *app2*... (or
    :func:`natron.getGuiInstance(index)<NatronGui.PyGuiApplication.getGuiInstance>`) instead.


The :doc:`PythonReference/NatronEngine/App` object is responsible for managing all information
relative to a project. This includes all the :doc:`nodes<PythonReference/NatronEngine/Effect>`,
project settings and render controls. See :ref:`this section <creatingNode>` to create
and control nodes.

Each node can have :doc:`parameters<PythonReference/NatronEngine/Param>` which are the controls
found in the settings panel of the node.

The same :doc:`PythonReference/NatronEngine/Param` class is also used for the project settings
and the application settings (preferences).

Overview of the classes
-----------------------

The two diagrams below show the main classes of the API. Arrows with a hollow head point from
a class to the class it inherits from. Plain arrows show which object gives access to which.

.. mermaid::
   :alt: PyGuiApplication inherits PyCoreApplication, which gives access to the App instances
         (app1, app2...) and to the AppSettings. GuiApp inherits App, and App inherits Group.
         Effect inherits Group and UserParamHolder. A Group contains Effects, and an Effect has
         Params, and may have a Roto context (Layers and BezierCurves, which inherit ItemBase)
         and a Tracker context (Tracks). GuiApp gives access to PyViewer, PyTabWidget and
         PyPanel; PyPanel and PyModalDialog inherit UserParamHolder.
   :caption: Applications, projects and nodes.

   classDiagram
       direction TB
       PyCoreApplication <|-- PyGuiApplication
       Group <|-- App
       App <|-- GuiApp
       Group <|-- Effect
       UserParamHolder <|-- Effect
       UserParamHolder <|-- PyPanel
       UserParamHolder <|-- PyModalDialog
       ItemBase <|-- Layer
       ItemBase <|-- BezierCurve
       PyCoreApplication --> "*" App : getInstance()
       PyCoreApplication --> AppSettings : getSettings()
       Group --> "*" Effect : getChildren()
       Effect --> "*" Param : getParams()
       Effect --> Roto : getRotoContext()
       Effect --> Tracker : getTrackerContext()
       Roto --> Layer : getBaseLayer()
       Layer --> "*" ItemBase : getChildren()
       Tracker --> "*" Track : getAllTracks()
       GuiApp --> PyViewer : getViewer()
       GuiApp --> PyTabWidget : getTabWidget()
       GuiApp --> PyPanel : getUserPanel()

.. mermaid::
   :alt: Param is the base class of all parameters. AnimatedParam, ButtonParam,
         SeparatorParam, GroupParam, PageParam and ParametricParam inherit Param. IntParam,
         DoubleParam, ColorParam, ChoiceParam, BooleanParam and StringParamBase inherit
         AnimatedParam. Int2DParam inherits IntParam and Int3DParam inherits Int2DParam;
         Double2DParam inherits DoubleParam and Double3DParam inherits Double2DParam.
         StringParam, FileParam, OutputFileParam and PathParam inherit StringParamBase.
   :caption: The parameter classes.

   classDiagram
       direction TB
       Param <|-- AnimatedParam
       Param <|-- ButtonParam
       Param <|-- SeparatorParam
       Param <|-- GroupParam
       Param <|-- PageParam
       Param <|-- ParametricParam
       AnimatedParam <|-- IntParam
       AnimatedParam <|-- DoubleParam
       AnimatedParam <|-- ColorParam
       AnimatedParam <|-- ChoiceParam
       AnimatedParam <|-- BooleanParam
       AnimatedParam <|-- StringParamBase
       IntParam <|-- Int2DParam
       Int2DParam <|-- Int3DParam
       DoubleParam <|-- Double2DParam
       Double2DParam <|-- Double3DParam
       StringParamBase <|-- StringParam
       StringParamBase <|-- FileParam
       StringParamBase <|-- OutputFileParam
       StringParamBase <|-- PathParam
