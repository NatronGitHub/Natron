.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Natron Environment Variables
============================

``NATRON_PLUGIN_PATH``: A semicolon-separated list of directories where to look for PyPlugs and Toolsets. Subdirectories are also searched, and symbolic links are followed.

``OFX_PLUGIN_PATH``: A semicolon-separated list of directories_ where to look for OpenFX plugin bundles. Subdirectories are also searched, and symbolic links are followed.

``PYTHONPATH``: semicolon-separated list of directories where to look for extra python modules. The Python modules should be compatible with Natron's embedded Python, and can be tested using the Python executable ``natron-python``, which is installed next to the Natron binary. Python modules can also be installed with pip_. For example, ``natron-python -m pip install numpy`` would install numpy for Natron.

``OCIO``: This variable can be used to point to the global OpenColorIO_ config file, e.g ``config.ocio``, and it supersedes the OpenColorIO setting in Natron's preferences.

``FONTCONFIG_PATH``: This variable may be used to override the default fontconfig_ configuration directory, which configures fonts used by :doc:`Text <plugins/net.fxarena.openfx.Text>` plug-ins.

``NATRON_DISK_CACHE_PATH``: The location where the Natron tile/image cache is stored. This overrides the "Disk cache path" preference. On Linux, the default location is the value of the environment variable ``XDG_CACHE_HOME`` followed by ``INRIA/Natron`` if set, else ``$HOME/.cache/INRIA/Natron``. On macOS, the default location is ``$HOME/Library/Caches/INRIA/Natron``. On Windows, the default location is ``C:\Documents and Settings\username\Local Settings\Application Data\cache\INRIA\Natron``.

.. _directories: http://openfx.sourceforge.net/Documentation/1.4/Reference/ch02s02.html#ArchitectureInstallingLocation

.. _fontconfig: https://www.freedesktop.org/software/fontconfig/fontconfig-user.html

.. _pip: https://pypi.org/project/pip/

.. _OpenColorIO: https://opencolorio.org/
