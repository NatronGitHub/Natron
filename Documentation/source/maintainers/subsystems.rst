.. _maint-subsystems:

Cross-Cutting Subsystems
========================

Some concerns are not owned by a single module but thread through the whole
application. This chapter gives each the short orientation a maintainer needs
before touching it.

Undo/redo
---------

Undo/redo spans Engine and Gui through the usual interface seam:

- ``Engine/UndoCommand.h`` declares an abstract ``UndoCommand`` (with
  ``redo()``/``undo()``); the Engine and the Python API can create and push
  commands without depending on Qt. ``NodeGuiI::pushUndoCommand`` is the seam
  the Engine uses to enqueue one.
- The Gui side drives Qt's ``QUndoStack``. There is not one global stack but
  **per-context stacks** so, for example, editing a curve and editing the node
  graph have independent histories. The concrete command sets live in
  ``NodeGraphUndoRedo``, ``KnobUndoCommand``, ``CurveEditorUndoRedo``,
  ``DopeSheetEditorUndoRedo`` (Gui) and ``RotoUndoCommand`` /
  ``TrackerUndoCommand`` (Engine).

When you add an editing operation, implement it as an undo command rather than
mutating state directly — otherwise it will silently break undo, the source of
bugs like `#839 <https://github.com/NatronGitHub/Natron/issues/839>`_.

Autosave and crash recovery
---------------------------

Natron protects the user's work with periodic autosave and out-of-process
resilience:

- ``Project::autoSave()`` / ``triggerAutoSave()`` write the project to a
  temporary autosave file (the threaded variant runs off the main thread).
  Autosave is triggered on inactivity and after significant edits. On startup
  Natron detects a leftover autosave and offers to recover it
  (``loadProject(..., attemptToLoadAutosave)``).
- ``ExistenceCheckerThread`` implements a liveness ping between cooperating
  processes; it emits ``otherProcessUnreachable()`` when the peer disappears.
  Combined with rendering in a separate process (``ProcessHandler``,
  ``ProcessInputChannel``), this is how a crash in a render process does not
  take the GUI down with it, and vice-versa.

If you change the project model or the save path, verify that autosave and
recovery still round-trip — this is a data-safety feature.

Crash reporting
---------------

Natron ships an **out-of-process crash reporter** built on
`Google Breakpad <https://chromium.googlesource.com/breakpad/breakpad/>`_. The
relevant trees are ``BreakpadClient`` (the in-process client that installs the
exception handler and writes a minidump), and ``CrashReporter`` /
``CrashReporterCLI`` (the separate reporter process that catches the dump and
lets the user submit it). It is enabled in the qmake build with
``CONFIG+=enable-breakpad`` (see ``breakpadpro.pri`` / ``breakpadclient.pri`` and
``README_breakpad.md``).

Because so many open issues are unreproducible "random crashes"
(`#557 <https://github.com/NatronGitHub/Natron/issues/557>`_), keeping this
pipeline working — dumps captured, symbols available, reports delivered — is one
of the highest-leverage things a maintainer can do (see :ref:`maint-todo`).

Multi-view and stereo
---------------------

Natron carries all views (e.g. left/right for stereo) through a single stream
rather than as separate projects. The key type is ``ViewIdx`` (``ViewIdx.h``),
which is threaded through the render actions (``renderRoI``, ``getFramesNeeded``,
``getRegionOfDefinition``) and folded into the cache key, so each view is
rendered and cached independently. The built-in ``OneViewNode`` extracts a
single view and ``JoinViewsNode`` combines views. When adding render code, plumb
``ViewIdx`` through exactly as the surrounding code does; dropping it silently
breaks stereo projects.

Internationalization (i18n)
---------------------------

User-facing strings are wrapped in Qt's ``tr()`` (over a hundred Gui files use
it) so they can be translated. Translation uses the standard Qt toolchain —
``lupdate`` to extract strings into ``.ts`` files, translators edit them, and
``lrelease`` compiles ``.qm`` files loaded at run time via ``QTranslator``.
See ``Documentation/I18N-HOWTO.txt`` for the project's specific process. Rules of
thumb:

- wrap human-readable strings in ``tr()``;
- never wrap script names, plug-in IDs or serialized keys (which must be stable
  and locale-independent);
- remember ``QT_NO_CAST_FROM_ASCII`` means non-translatable literals still need
  ``QString::fromUtf8``.

Resources
---------

Icons, shaders, fonts, stylesheets, OpenColorIO configs, MIME data and bundled
PyPlugs are embedded through Qt's resource system: ``Gui/GuiResources.qrc``
indexes the files under ``Gui/Resources/`` (``Images``, ``Fonts``,
``Stylesheets``, ``PyPlugs``, …), and CMake's ``AUTORCC`` (or qmake's rcc step)
compiles them into the binary. Reference embedded files with ``:/`` paths. Add
new assets to the ``.qrc`` so they are packaged.

Logging
-------

``Engine/Log`` (with ``LogEntry``) is a lightweight, process-wide logging
facility with static ``print`` methods; in release builds it compiles to no-ops
unless logging is enabled. The GUI surfaces log output and errors through
``LogWindow`` and the ``MessageBox`` helpers. Use this facility (not raw
``std::cout``) for diagnostics that should be visible to users and captured in
bug reports.

The plug-in cache
-----------------

Scanning every OpenFX bundle on every launch would be slow, so the OpenFX host
persists a **plug-in cache** describing the discovered plug-ins and their
descriptors; on startup it is validated against the bundles on disk and only
changed plug-ins are re-read. The cache lives in the host support layer
(``OFX::Host::ImageEffect::PluginCache``) driven by ``OfxHost``. If a plug-in
appears stale or missing after an update, a corrupt or out-of-date plug-in cache
is a likely cause — clearing it forces a full rescan.

GPU rendering path
------------------

Beyond the OpenGL *viewer*, effects can render on the GPU. Natron implements the
OpenFX OpenGL-render suite so capable plug-ins render into GL textures, using the
off-screen contexts from ``OSGLContext`` / ``GPUContextPool``
(:ref:`maint-rendering`). Natron's own GL helpers include ``GLShader``
(shader-program wrapper) and ``DefaultShaders``; the Shadertoy node is a notable
GPU effect. Whether a given render runs on CPU or GPU depends on plug-in support
and the user's settings.
