.. _startupScripts:

Start-up scripts
================

On start-up Natron will run different start-up scripts to let you setup anything like callbacks,
menus, etc...

There are 2 different initialization scripts that Natron will look for in the :ref:`search paths<natronPath>`.

    * **init.py**

        This script is always run and should only initialize non-GUI stuff. You may not use
        it to initialize e.g. new menus or windows. Generally this is a good place to initialize
        all the callbacks that you may want to use in your projects.

    * **initGui.py**

        This script is only run in GUI mode (that is with the user interface). It should
        initialize all gui-specific stuff like new menus or windows.

All the scripts with these names that are found in the search paths are run, in this order:

- the :ref:`search paths<natronPath>` are processed one after the other, in their order;
- each search path is explored depth-first: in each directory, Natron first runs **init.py**, then
  **initGui.py** (in GUI mode only), and then explores the sub-directories, in alphabetical order.

So the initGui.py of a directory runs before the init.py of the next directory or search path.
All the startup scripts run before any PyPlug is loaded, so a startup script can define
functions and modules that the PyPlugs use.

The diagram shows the whole startup, from the command line to the project, and where the
startup scripts and PyPlugs are loaded.

.. mermaid::
   :alt: Natron or NatronRenderer parses the command line, initializes Python and the
         NatronEngine and NatronGui modules, reads the preferences and applies the --setting
         options, then loads the built-in nodes and the OpenFX plug-ins. It then adds every
         plug-in search path to sys.path, imports qtpy, runs init.py and initGui.py for each
         directory, and imports and registers the PyPlugs, skipping those that import NatronGui
         in background mode. Finally it creates the project instance app1, runs the -c commands,
         loads the project or the script, runs the --onload script, and renders or starts the
         interpreter.
   :caption: Startup of Natron and NatronRenderer, and where the startup scripts run.

   flowchart TD
       start(["Natron / NatronRenderer"]) --> cl["Parse the command line"]
       cl --> py["Initialize Python,<br/>the NatronEngine (and NatronGui) modules"]
       py --> set["Read the preferences<br/>(unless --no-settings),<br/>apply --setting and --opengl"]
       set --> plug["Load the built-in nodes<br/>and the OpenFX plug-ins"]
       plug --> path["Add every plug-in search path<br/>and sub-directory to sys.path;<br/>import qtpy"]
       path --> init["For each search path, depth-first:<br/>run init.py, then initGui.py (GUI only)"]
       init --> pyplugs["Import the PyPlugs found<br/>(skip NatronGui imports in background)<br/>and register them"]
       pyplugs --> app["Create the project instance:<br/>app1 (and app in background)"]
       app --> cmd["Run the -c commands"]
       cmd --> load["Load the .ntp project,<br/>or run the .py script"]
       load --> onload["Run the --onload script"]
       onload --> mode{"Mode?"}
       mode -- "background" --> render["Apply -w, -o, then -i, and render"]
       mode -- "interpreter (-t)" --> repl["Read Python commands<br/>from the standard input"]
       mode -- "GUI" --> gui["Show the main window"]


The startup scripts are run in the ``__main__`` Python module, so the functions they define are
referenced without a module prefix, e.g. ``NatronEngine.natron.setOnProjectCreatedCallback("myCallback")``.

In GUI mode, Natron prints a line such as ``Info: init.py script found and loaded at ...``
for each startup script that it runs, in the terminal and in the splash screen.
If a startup script raises an exception, the error is written to the terminal and to the
error log (:menuselection:`Display --> Show Project Errors Log...`), and Natron goes on with the other scripts.

.. warning::

    This is important that the 2 scripts above are named **init.py** and **initGui.py**
    otherwise they will not be loaded.

.. warning::

    These scripts are run well before any :ref:`application instance<App>` (i.e: project) is created.
    You should therefore not run any function directly that might rely on the *app* variable (or *app1*, etc...).
    However you're free to define classes and functions that may rely on these variable being declared, but that
    will be called only later on, when a project will actually be created.

Examples
--------


initGui.py
^^^^^^^^^^^

A complete example of a **initGui.py** can be found :ref:`here<sourcecodeEx>` .


.. _initExample:

init.py
^^^^^^^

Here is an example of a **init.py** script, featuring:

- Formats addition to the project
- Modifications of the default values of parameters for nodes
- PyPlug search paths modifications

.. literalinclude:: init.py
