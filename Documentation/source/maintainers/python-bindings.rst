.. _maint-python:

Python Bindings (Shiboken / PySide)
===================================

Natron embeds a Python 3 interpreter and exposes much of its API to Python. This
powers parameter expressions, user scripts and callbacks, PyPlugs, and custom
GUI panels — everything documented in the :ref:`developers-guide`. This chapter
covers the *machinery* that produces those bindings. The build regenerates them
whenever a header or a typesystem file changes.

The pieces
----------

- **The** ``Py*`` **C++ classes** (``Engine/PyNode.*``, ``PyParameter.*``,
  ``PyRoto.*``, ``PyTracker.*``, ``PyAppInstance.*``, ``PyNodeGroup.*``,
  ``PyExprUtils.*``, ``PyGlobalFunctions.*``; ``Gui/PyGuiApp.*``,
  ``PyGlobalGui.*``, ``PythonPanels.*``). These are deliberately small, stable,
  scripting-friendly facades over the engine/GUI objects. They live in the
  ``NATRON_PYTHON_NAMESPACE`` (see :ref:`maint-design`). The Python API surface
  is defined by these classes — not by the whole engine.
- **Shiboken** — the binding generator (part of PySide). It reads the C++
  headers plus a *typesystem* XML file and emits C++ "wrapper" sources that
  expose the classes to Python.
- **The typesystem files** — ``Engine/typesystem_engine.xml`` (package
  ``NatronEngine``) and ``Gui/typesystem_natronGui.xml`` (package
  ``NatronGui``). They tell Shiboken which classes/functions to expose, how to
  map types, what to rename, and what to skip. ``typesystem_engine.xml`` loads
  PySide's ``typesystem_core.xml``; ``typesystem_natronGui.xml`` loads PySide's
  ``typesystem_gui.xml`` and ``typesystem_widgets.xml``, and
  ``typesystem_engine.xml``. When you add or change a ``Py*`` class, you edit
  the matching typesystem file.
- **The global headers** — the header that Shiboken parses.
  ``Engine/Pyside_Engine_Python.h`` and ``Gui/Pyside_Gui_Python.h`` include
  ``Python.h``, the PySide global header and the ``Py*`` headers. CMake passes
  ``PySide2_Engine_Python.h`` or ``PySide6_Engine_Python.h`` (and the ``Gui``
  equivalents), chosen from the PySide major version: they define
  ``SBK2_RUN`` or ``SBK6_RUN`` for version-specific tweaks (the PySide6 one
  also hides the Boost serialization macros from Shiboken), then include the
  common header.
- **The generated wrappers** — ``natronengine_module_wrapper.cpp``, one
  ``<class>_wrapper.cpp`` per exposed class, and the matching headers, written
  into the build directory. They are build artifacts, never edited by hand.

.. _maint-python-generation:

How the bindings are generated
------------------------------

In the CMake build (``Engine/CMakeLists.txt`` and ``Gui/CMakeLists.txt``):

.. mermaid::
   :alt: At configure time, sourceList.py reads typesystem_engine.xml and lists
         the wrapper files. At build time, shiboken2 or shiboken6 reads the
         global header, which includes the Py* headers, and the typesystem
         file, which loads PySide typesystem_core.xml, and writes the wrappers
         into the build directory. sourceCleanup.py post-processes them, and
         they are compiled into the NatronEngine static library. At run time,
         AppManager::initPython registers PyInit_NatronEngine. The Gui module
         is built the same way.
   :caption: Generating the NatronEngine module (the NatronGui module is built
             the same way).

   flowchart LR
       subgraph inputs ["Sources"]
           PY["Engine/Py*.h"]
           GH["PySide2_Engine_Python.h<br/>or PySide6_Engine_Python.h"]
           TS["typesystem_engine.xml"]
           PTS["PySide typesystem_core.xml"]
       end
       SL["tools/utils/sourceList.py<br/>(configure time)"]
       SB["shiboken2 / shiboken6<br/>(build time)"]
       OUT["build/Engine/Qt5 or Qt6/NatronEngine/<br/>natronengine_module_wrapper.cpp<br/>*_wrapper.cpp"]
       SC["tools/utils/sourceCleanup.py"]
       LIB["NatronEngine static library"]
       RT["AppManager::initPython()<br/>PyImport_AppendInittab(PyInit_NatronEngine)"]
       TS --> SL
       SL -->|"list of wrapper files"| OUT
       PY --> GH
       GH --> SB
       TS --> SB
       PTS -.->|"loaded by"| TS
       SB --> OUT
       OUT --> SC
       SC -->|"compiled into"| LIB
       LIB -->|"at run time"| RT

1. At configure time, ``tools/utils/sourceList.py`` reads the typesystem file
   and lists the files that Shiboken will write: the module wrapper plus one
   ``<name>_wrapper.cpp`` for each ``object-type``, ``value-type`` and
   ``namespace-type``, under ``<build>/Engine/Qt5/NatronEngine/`` (or
   ``Qt6``; ``<build>/Gui/Qt5/NatronGui/`` for the Gui). This is why
   re-running CMake is needed after adding a type to a typesystem file.
2. At build time, a custom command runs ``shiboken2`` or ``shiboken6`` on the
   global header and the typesystem file, with ``-std=c++17`` (Qt 5) or
   ``-std=c++20`` (Qt 6; ``NATRON_SHIBOKEN_STD`` in ``CMakeLists.txt``),
   ``--enable-parent-ctor-heuristic``, ``--use-isnull-as-nb_nonzero``,
   ``--avoid-protected-hack``, ``--enable-pyside-extensions``, the include
   paths, and the PySide typesystem directory (``-T``; the Gui adds
   ``-T../Engine`` to find ``typesystem_engine.xml``). It runs again whenever a
   header of the module or the typesystem file changes.
3. The same command then runs ``tools/utils/sourceCleanup.py`` on the output.
   It removes the leading ``::`` that Shiboken writes before type names that
   are not in the ``Natron`` namespace, and, in the ``NatronGui`` module
   wrapper, removes the declarations of the QtCore and ``NatronEngine`` type
   tables and renames ``cleanTypesAttributes`` to
   ``cleanGuiTypesAttributes``, so that the two modules can be linked into the
   same executable.
4. The wrappers are compiled into the ``NatronEngine`` and ``NatronGui``
   static libraries.

The qmake build cannot run Shiboken: the release script runs the same steps
before qmake (``tools/jenkins/build-natron.sh``, Qt 5 only), with
``shiboken2``, then ``sourceCleanup.py``. It also adds ``Shiboken/`` to the
typesystem path; that directory only holds an empty placeholder
``typesystem_widgets.xml`` left over from Shiboken 1.2.

At run time, ``AppManager::initPython()`` (``Engine/AppManager.cpp``) calls
``initBuiltinPythonModules()`` before the interpreter starts: it registers
``PyInit_NatronEngine`` (generated in the module wrapper) with
``PyImport_AppendInittab``, and ``GuiApplicationManager`` adds
``PyInit_NatronGui`` (``Gui/GuiApplicationManager10.cpp``). The interpreter
is then started (``initializePython3`` in ``Global/PythonUtils.cpp``) and
``NatronEngine`` is imported. The modules are built into the executables, not
loaded from files.

Qt 5 vs Qt 6: two toolchains
----------------------------

The binding toolchain is tied to the Qt version:

- Qt 5 builds use **Shiboken2 / PySide2**.
- Qt 6 builds use **Shiboken6 / PySide6**.

The CMake build selects between them from the ``NATRON_QT6`` option (see
:ref:`maint-building`), with the matching ``PySide2_*`` or ``PySide6_*`` global
headers. The bindings must be **regenerated** with
the matching Shiboken when switching Qt major versions. See :ref:`maint-qt6`.

Working on the bindings
-----------------------

- To expose a new method or class to Python: add/adjust the ``Py*`` facade in
  C++, then declare it in the relevant ``typesystem_*.xml``, then rebuild so
  Shiboken regenerates the wrappers (re-run CMake if you added a type, see
  :ref:`maint-python-generation`).
- Keep the ``Py*`` API **stable and minimal**. User scripts, PyPlugs and the
  official tutorials depend on it; breaking it breaks users' projects just as
  surely as breaking the serialization format.
- The reference documentation of the Python API
  (``Documentation/source/devel/PythonReference``, for example
  :doc:`../devel/PythonReference/NatronEngine/App`) is written by hand, not
  generated from the bindings: update it in the same pull request as the
  ``Py*`` change.
- To add a third-party Python dependency for scripts, callbacks or PyPlugs,
  install it into Natron's bundled interpreter with
  ``natron-python -m pip install <package>`` rather than the system Python — this
  keeps the package on the same Python that Natron actually runs.

Threads and the GIL
-------------------

Python runs on several threads: scripts and callbacks on the main thread, Python
expressions and render callbacks on the render threads. Each one holds the Python
GIL while it runs Python, through ``PythonGILLocker`` (``Engine/AppManager.h``).

- **Lock order: a lock held while running Python comes before the GIL.** Some locks
  are held across code that may take the GIL: a render action, which may evaluate a
  Python expression, holds the render lock of its effect, the clip preferences lock
  (``preferencesLock``, for reading) and the OFX suite mutexes it locked. A thread
  that holds the GIL must not block on such a lock, nor wait for another thread (a
  render, the multi-thread suite): it would deadlock with a thread that holds the lock
  and waits for the GIL. Short locks that are never held while taking the GIL (e.g.
  the value of a knob) are fine. Release the GIL first with ``PythonGILUnlocker``, a
  no-op on threads that don't hold it, as in ``OfxClipInstance::getInputImageInternal()`` and
  ``OfxHost::multiThread()`` (#1653), ``OfxEffectInstance::onMetadataRefreshed()``
  and ``OfxHost::mutexLock()`` (#1862), or ``AppInstance::startWritersRendering()``.
  See `CP.21 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-lock>`_
  and `CP.22 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-unknown>`_.
- No Python API may be called while a ``PythonGILUnlocker`` exists, except through a
  ``PythonGILLocker``. Declare the unlocker before the lock it protects, so that the GIL
  is taken again after the lock is released.
- Tests: ``Tests/OfxInstanceChangedGIL_Test.cpp`` holds the GIL as a script does
  while another thread needs it, with a watchdog that aborts on a deadlock.
