.. _pythonEnvironment:

The Python environment and extra modules
========================================

Natron embeds its own Python 3 interpreter. It doesn't use a Python installed on your system.
This page explains where Natron looks for Python modules, and how to add modules such as
numpy to it.

The bundled Python
------------------

The Natron 2.6 binaries ship Python 3.11 in Qt 5 builds and Python 3.13 in Qt 6 builds. Check
the version of your build with ``import sys; print(sys.version)`` in the Script Editor. The
version matters for modules with compiled code: they must be built for that Python version.

Natron sets up the Python path (``sys.path``) at startup, in this order:

1. the Python standard library bundled with Natron, and its ``site-packages`` directory, where
   the Natron binaries install qtpy and the PySide module (PySide2 or PySide6);
2. the ``Plugins`` directory of the Natron installation;
3. the directories of the ``PYTHONPATH`` environment variable (separated by ``;`` on Windows
   and ``:`` on macOS and Linux);
4. all the directories of the :ref:`plug-in search paths<natronPath>` and their sub-directories,
   which are added when the plug-ins are loaded.

The **user site-packages directory is disabled** (Natron sets ``PYTHONNOUSERSITE``): modules
installed with ``pip install --user`` for a system Python are not visible in Natron, because they
could conflict with the bundled packages.

natron-python
-------------

``natron-python`` (``natron-python.exe`` on Windows) is installed next to the Natron executable.
It is a plain Python interpreter that uses the same Python and the same paths as Natron, so you
can use it to test whether a module can be imported, or to run pip::

    natron-python -c "import numpy; print(numpy.__version__)"
    natron-python -m pip install numpy

``natron-python -m pip install`` installs into the ``site-packages`` directory of the Natron
installation, so you need write access to it. On a shared or read-only installation, install the
modules into another directory and add it to ``PYTHONPATH``, or to a plug-in search path::

    natron-python -m pip install --target /studio/natron-python-modules numpy

The Natron modules, ``NatronEngine`` and ``NatronGui``, are built into the Natron executables: they
can't be imported in natron-python. To test code that uses them without the user interface,
use the interpreter mode, ``NatronRenderer -t`` (see :ref:`debuggingPython`).

Qt bindings
-----------

The Qt bindings are PySide2 in Qt 5 builds of Natron, and PySide6 in Qt 6 builds. Natron
bundles `qtpy <https://github.com/spyder-ide/qtpy>`_, a small layer that gives the same module
names with both, and imports it at startup. Write your Qt code with qtpy, using the Qt 5
module layout (QtWidgets for widgets)::

    from qtpy import QtCore, QtGui, QtWidgets

    label = QtWidgets.QLabel("Hello")

Don't install another PySide or PyQt into Natron's Python: the bindings must be the ones that
Natron was built with.
