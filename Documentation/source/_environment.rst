.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Environment Variables
=====================

What are Environment Variables?
-------------------------------

`Environment variables <https://en.wikipedia.org/wiki/Environment_variable>`_ are global system variables accessible by all the processes running under the Operating System (OS). Environment variables are useful to store system-wide values such as the directories to search for the executable programs (``PATH``) and the OS version.

How do I set an environment variable?
-------------------------------------

Linux
^^^^^

To set an environment variable on Linux, enter the following command at a shell prompt, according to which shell you are using:

- bash/ksh/zsh: ``export variable=value``
- csh/tcsh: ``setenv variable value``

where variable is the name of the environment variable (such as ``OFX_PLUGIN_PATH``) and value is the value you want to assign to the variable, (such as ``/opt/OFX/Plugins``). To find out what environment variables are set, use the ``env`` command. To remove a variable from the environment, use the following commands:

- bash/ksh/zsh: ``export -n variable``
- csh/tcsh: ``unsetenv variable``

To set permanently an environment variable, add the command to your shell’s startup script in your home directory. For Bash, this is usually ``~/.bashrc``. Changes in these startup scripts don’t affect shell instances already started; try opening a new terminal window to get the new settings, or refresh the curent settings using ``source ~/.bashrc``.

Windows
^^^^^^^

You can create or change environment variables in the Environment Variables dialog box. If you are adding to the ``PATH`` environment variable or any environment variable that takes multiple values, you should separate each value with a semicolon (;).

Windows 8 and Windows 10
""""""""""""""""""""""""

To open the Environment Variables dialog box:

1. In Search, search for and then select: Edit environment variables for your account

To create a new environment variable:

1. In the User variables section, click New to open the New User Variable dialog box.
2. Enter the name of the variable and its value, and click OK. The variable is added to the User variables section of the Environment Variables dialog box.
3. Click OK in the Environment Variables dialog box.

To modify an existing environment variable:

1. In the User variables section, select the environment variable you want to modify.
2. Click Edit to open the Edit User Variable dialog box.
3. Change the value of the variable and click OK. The variable is updated in the User variables section of the Environment Variables dialog box.

When you have finished creating or editing environment variables, click OK in the Environment Variables dialog box to save the values.

Windows 7
"""""""""

To open the Environment Variables dialog box:

1. Click Start, then click Control Panel
2. Click User Accounts.
3. Click User Accounts again.
4. In the Task side pane on the left, click Change my environment variables to open the Environment Variables dialog box opens.

To create a new environment variable:

1. In the User variables section, click New to open the New User Variable dialog box.
2. Enter the name of the variable and its value, and click OK. The variable is added to the User variables section of the Environment Variables dialog box.
3. Click OK in the Environment Variables dialog box.

To modify an existing environment variable:

1. In the User variables section, select the environment variable you want to modify.
2. Click Edit to open the Edit User Variable dialog box opens.
3. Change the value of the variable and click OK. The variable is updated in the User variables section of the Environment Variables dialog box.

When you have finished creating or editing environment variables, click OK in the Environment Variables dialog box to save the values. You can then close the Control Panel.

macOS
^^^^^

To set an environment variable on macOS, open a terminal window. If you are setting the environment variable to run jobs from the command line, use the following command:

``export variable=value``

where variable is the name of the environment variable (such as ``OFX_PLUGIN_PATH``) and value is the value you want to assign to the variable, (such as ``/opt/OFX/Plugins``). You can find out which environment variables have been set with the ``env`` command.

If you are setting the environment variable globally to use with applications, use the commands given below. The environment variables set by these commands are inherited by any shell or application.

macOS newer than 10.10
""""""""""""""""""""""

See `this article <https://apple.stackexchange.com/questions/106355/setting-the-system-wide-path-environment-variable-in-mavericks>`_ for instructions on how to create a "plist" file to store system-wide environment variables in newer versions of macOS.

MacOS X 10.10
"""""""""""""

To set an environment variable, enter the following command:

``launchctl setenv variable "value"``

To find out if an environment variable is set, use the following command:

``launchctl getenv variable``

To clear an environment variable, use the following command:

``launchctl unsetenv variable``

Natron Environment Variables
----------------------------

``NATRON_PLUGIN_PATH``: A semicolon-separated list of directories where to look for PyPlugs and Toolsets. Subdirectories are also searched, and symbolic links are followed.

``OFX_PLUGIN_PATH``: A semicolon-separated list of directories_ where to look for OpenFX plugin bundles. Subdirectories are also searched, and symbolic links are followed.

``PYTHONPATH``: semicolon-separated list of directories where to look for extra python modules. The Python modules should be compatible with Natron's embedded Python, and can be tested using the Python executable ``natron-python``, which is installed next to the Natron binary. Python modules can also be installed with pip_. For example, ``natron-python -m pip install numpy`` would install numpy for Natron.

``OCIO``: This variable can be used to point to the global OpenColorIO_ config file, e.g ``config.ocio``, and it supersedes the OpenColorIO setting in Natron's preferences.

``FONTCONFIG_PATH``: This variable may be used to override the default fontconfig_ configuration directory, which configures fonts used by :doc:`Text <plugins/net.fxarena.openfx.Text>` plug-ins.

``NATRON_DISK_CACHE_PATH``: The location where the Natron tile/image cache is stored. This overrides the "Disk cache path" preference. On Linux, the default location is ``$XDG_CACHE_HOME/INRIA/Natron`` if the environment variable ``XDG_CACHE_HOME`` is set, else ``$HOME/.cache/INRIA/Natron``. On macOS, the default location is ``$HOME/Library/Caches/INRIA/Natron``. On Windows, the default location is ``C:\Documents and Settings\%USERNAME%\Local Settings\Application Data\cache\INRIA\Natron``.

.. _directories: http://openfx.sourceforge.net/Documentation/1.4/Reference/ch02s02.html#ArchitectureInstallingLocation

.. _fontconfig: https://www.freedesktop.org/software/fontconfig/fontconfig-user.html

.. _pip: https://pypi.org/project/pip/

.. _OpenColorIO: https://opencolorio.org/
