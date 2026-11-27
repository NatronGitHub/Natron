.. _pythonEnvironment:

The Python environment and extra modules
========================================

Natron embeds its own Python 3 interpreter. It doesn't use a Python installed on your system.
This page explains where Natron looks for Python modules, and how to add modules such as
numpy to it.

The bundled Python
------------------

The Python 3 version depends on your Natron build: check it with
``import sys; print(sys.version)`` in the Script Editor. The version matters for modules with compiled code: they must be built for that Python version.

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

Bundled packages
----------------

Besides the Python standard library and the PySide module, the Natron binaries bundle these
third-party packages in the ``site-packages`` directory:

- `pip <https://pip.pypa.io>`_, to install other packages (see below);
- `qtpy <https://github.com/spyder-ide/qtpy>`_, see `Qt bindings`_;
- `six <https://six.readthedocs.io>`_, Python 2 and 3 compatibility utilities;
- `psutil <https://github.com/giampaolo/psutil>`_, on Linux and macOS only;
- the packages that these need, such as ``packaging`` (for qtpy), and ``setuptools`` and
  ``wheel`` where pip's installer adds them (with Python 3.11).

The ``future`` package (python-future), which earlier versions bundled for scripts written for
Python 2, is no longer bundled since Natron 2.6. A script that uses it fails with an error such as
``ModuleNotFoundError: No module named 'past'`` (or ``'future'``, ``'builtins'``,
``'libfuturize'``, ``'libpasteurize'``). Install it like any other package:
``natron-python -m pip install future``.

natron-python
-------------

``natron-python`` is installed next to the Natron executable:

- Linux: ``<install>/bin/natron-python``, where ``<install>`` is the directory that contains the
  ``Natron`` launch script;
- macOS: ``Natron.app/Contents/MacOS/natron-python``;
- Windows: ``<install>\bin\natron-python.exe``, for example
  ``C:\Program Files\Natron\bin\natron-python.exe``.

It is a plain Python interpreter that uses the same Python and the same paths as Natron, so you
can use it to test whether a module can be imported, or to run pip::

    natron-python -c "import numpy; print(numpy.__version__)"
    natron-python -m pip install numpy

The Natron modules, ``NatronEngine`` and ``NatronGui``, are built into the Natron executables: they
can't be imported in natron-python. To test code that uses them without the user interface,
use the interpreter mode, ``NatronRenderer -t`` (see :ref:`debuggingPython`).

Installing extra packages from the command line
-----------------------------------------------

Run pip with natron-python, giving its full path if its directory isn't on ``PATH``. Without
other options, ``natron-python -m pip install`` installs into the ``site-packages`` directory of
the Natron installation, so you need write access to it: on Windows, an installation in
``C:\Program Files`` needs an administrator prompt. On a shared or read-only installation,
install the packages into another directory with ``--target``, and add that directory to
``PYTHONPATH``::

    # Linux, Natron extracted to /opt/Natron
    /opt/Natron/bin/natron-python -m pip install --target ~/natron-python-modules numpy
    export PYTHONPATH=~/natron-python-modules

    # macOS
    /Applications/Natron.app/Contents/MacOS/natron-python -m pip install --target ~/natron-python-modules numpy
    export PYTHONPATH=~/natron-python-modules

    rem Windows (cmd.exe)
    "C:\Program Files\Natron\bin\natron-python.exe" -m pip install --target %USERPROFILE%\natron-python-modules numpy
    set PYTHONPATH=%USERPROFILE%\natron-python-modules

``export`` and ``set`` only reach a Natron started from the same shell. For a Natron started
from the desktop (Finder or Dock, Start menu, a desktop launcher), set ``PYTHONPATH`` for the
session instead (for example ``setx PYTHONPATH %USERPROFILE%\natron-python-modules`` on Windows,
or ``launchctl setenv PYTHONPATH ~/natron-python-modules`` on macOS, not tested), or add the
directory to ``sys.path`` in ``init.py`` (see :ref:`startupScripts`).

On macOS, installing into ``Natron.app`` modifies the signed application bundle, so prefer
``--target`` there.

Don't use a plug-in search path as the ``--target`` directory: Natron adds every sub-directory
of the plug-in search paths to ``sys.path``, so the sub-directories of the installed packages
could hide other modules.

Packages with compiled code must have a wheel (or be buildable) for Natron's Python version and
platform; pure-Python packages work everywhere.

The Linux and Windows recipes are not tested.

Installing extra packages from the Script Editor
------------------------------------------------

In Natron, ``sys.executable`` is the Natron executable itself (``Natron`` or ``NatronRenderer``),
not a Python interpreter: ``sys.executable -m pip`` would start another Natron. Run the
natron-python that is next to the Natron executable instead, then add the target directory to
``sys.path`` and import the package (``--upgrade`` replaces an earlier copy)::

    import importlib
    import os
    import subprocess
    import sys

    from qtpy import QtCore

    exe = "natron-python.exe" if sys.platform == "win32" else "natron-python"
    natron_python = os.path.join(
        QtCore.QCoreApplication.applicationDirPath(), exe)
    target = os.path.join(os.path.expanduser("~"), "natron-python-modules")

    result = subprocess.run(
        [natron_python, "-m", "pip", "install", "--upgrade",
         "--target", target, "tabulate"],
        capture_output=True, text=True,
        # no console window on Windows
        creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    print(result.stdout, result.stderr)
    result.check_returncode()

    if target not in sys.path:
        sys.path.append(target)
    importlib.invalidate_caches()
    import tabulate
    print(tabulate.tabulate([["a", 1], ["b", 2]]))

Natron doesn't respond while pip runs. ``importlib.invalidate_caches()`` makes Python see the
files that pip has just written. The ``sys.path`` change lasts until Natron quits: to use the
packages in the next sessions, add the target directory to ``PYTHONPATH`` (see above), or append
it to ``sys.path`` in ``init.py`` (see :ref:`startupScripts`).

This recipe is tested with ``NatronRenderer -t`` on macOS; not tested in the Script Editor of the
Natron user interface, nor on Linux or Windows.

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
