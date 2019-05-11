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

All the scripts with the above name found in the search paths will be run in the order
of the :ref:`search paths<natronPath>`.

.. warning::

    This is important that the 2 scripts above are named **init.py** and **initGui.py**
    otherwise they will not be loaded.

.. warning::

    These scripts are run well before any :ref:`application instance<App>` (i.e: project) is created.
    You should therefore not run any function directly that might rely on the *app* variable (or *app1*, etc...).
    However you're free to define classes and functions that may rely on these variable being declared, but that
    will be called only later on, when a project will actually be created.

Examples
=========


initGui.py
-----------

A complete example of a **iniGui.py** can be found :ref:`here<sourcecodeEx>` .


.. _initExample:

init.py
-------

Here is an example of a **init.py** script, featuring:

- Formats addition to the project
- Modifications of the default values of parameters for nodes
- PyPlug search paths modifications

.. literalinclude:: init.py
