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

    Note that when running scripts in the *Script Editor*, the application is running in GUI
    mode hence the *app* variable is not declared.


The :doc:`PythonReference/NatronEngine/App` object is responsible for managing all information
relative to a project. This includes all the :doc:`nodes<PythonReference/NatronEngine/Effect>`,
project settings and render controls. See :ref:`this section <creatingNode>` to create
and control nodes.

Each node can have :doc:`parameters<PythonReference/NatronEngine/Param>` which are the controls
found in the settings panel of the node.

The same :doc:`PythonReference/NatronEngine/Param` class is also used for the project settings
and the application settings (preferences).
