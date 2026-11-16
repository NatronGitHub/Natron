.. _maint-contributing:

Contributing and Workflow
=========================

This chapter covers the mechanics of getting a change accepted and of building
this manual.

Coordination and communication
------------------------------

Development is coordinated through the `GitHub issue tracker
<https://github.com/NatronGitHub/Natron/issues>`_ and discussed on the
`pixls.us Natron forum <https://discuss.pixls.us/c/software/natron>`_ and the
project Discord. Natron is actively "looking for developers and maintainers"; if
you intend to work on something substantial, open or comment on an issue first so
effort is not duplicated.

Branches
--------

- ``master`` is the main development branch.
- Each supported stable release has its own branch, named ``RB-x.y`` (for
  example ``RB-2.6``). Bug fixes for a release go onto that branch.
- Fork the repository, make your change on a topic branch, and open a pull
  request against the appropriate branch (usually the current ``RB-x.y`` for
  fixes, or ``master`` for new development).

Follow the existing pull-request template
(``.github/PULL_REQUEST_TEMPLATE.md``) and reference the issue(s) your change
addresses.

Code style
----------

Style is enforced automatically by **astyle** through the pre-commit hook, as
described in :ref:`maint-building`. Install the hook, and if it rejects a
commit, run astyle with the pinned flags and re-stage. Pull requests that do not
match the style can still be merged after a maintainer reformats them, but you
save everyone time by formatting first. Beyond formatting, match the
surrounding code: the idioms in :ref:`maint-design` (namespace macros,
``Fwd`` typedefs, PIMPL, the ``…I`` interfaces, ``shared_ptr`` ownership rules)
are conventions, not suggestions.

Tests
-----

The ``Tests`` module (Google Test / Google Mock) covers focused pieces of the
engine: curves, LUT/color, images, hashing, tracking, the file-system model and
GL contexts. Build and run the suite before opening a pull request. When you fix
a bug that is unit-testable — anything in the color, image, curve, hashing or
geometry code — add a regression test. Rendering and GUI behavior are harder to
unit-test; for those, describe your manual test steps in the pull request.

Two changes deserve special care because they can silently break users:

- **Serialization** (``*Serialization.h``): version every change and keep a
  read path for old projects (see :ref:`maint-design`).
- **The Python API** (``Py*`` classes and typesystems): keep it backward
  compatible (see :ref:`maint-python`).

Debugging and profiling
-----------------------

The tools for diagnosing a Natron bug are spread across the code base; this is
the consolidated list (several are referenced from the P0 leads in
:ref:`maint-issue-triage`):

- **Debug build.** Build with ``CMAKE_BUILD_TYPE=Debug`` (or qmake
  ``CONFIG+=debug``). This defines ``DEBUG`` and enables **floating-point
  exception trapping** at startup (``Global/FloatingPointExceptions.h``), so a
  stray ``NaN`` or division by zero aborts at the source instead of silently
  polluting the image pipeline.
- **AddressSanitizer.** The qmake build supports ``CONFIG+=addresssanitizer``
  (see the notes at the bottom of ``Project.pro``) — the fastest way into
  use-after-free and destruction-order bugs such as the teardown crashes
  (#795/#1029/#1057).
- **Thread sanitizer / stuck-thread backtraces.** For deadlocks and stalls
  (#248), run under a thread sanitizer or attach a debugger and dump all thread
  backtraces; the suspects are the scheduler condition variables, cache-entry
  locks, and the "being rendered elsewhere" wait (see :ref:`maint-rendering`).
- **Headless reproduction.** Reproduce render bugs with ``NatronRenderer`` (no
  GUI) to isolate engine issues from GUI ones and to run under sanitizers
  cleanly.
- **Render statistics.** ``RenderStats`` (surfaced in the GUI's
  ``RenderStatsDialog``) reports per-node render times and cache hits/misses —
  use it to find where a slow or repeated render is spent.
- **Logging.** Route diagnostics through ``Engine/Log`` (visible in the GUI
  ``LogWindow``) rather than raw ``std::cout`` so they appear in user bug
  reports (see :ref:`maint-subsystems`).
- **Crashes in the field.** Usable Breakpad minidumps are the way to diagnose
  the "random crash" tail; see the crash-reporting notes in
  :ref:`maint-subsystems`.

Where to start
--------------

The project README grades starter tasks by difficulty. A reasonable on-ramp is:
build Natron locally; fix a small, well-scoped bug labeled ``difficulty:easy``;
then take on a ``difficulty:medium`` item. The :ref:`maint-issue-triage` chapter
lists concrete good-first-issue candidates.

Building this manual
--------------------

The documentation is `Sphinx <https://www.sphinx-doc.org/>`_ using the Read the
Docs theme; sources are under ``Documentation/source``. Build the HTML with::

    cd Documentation
    sphinx-build -b html source html

and the PDF with::

    sphinx-build -b latex source pdf
    (cd pdf; pdflatex Natron)

Two rules from ``Documentation/README-Natron-documentation.md`` are important:

- **Do not hand-edit** ``index.rst``, files whose names start with ``_``, or
  anything under ``plugins/``. The per-node pages in ``plugins/`` are generated
  from the plug-ins themselves with ``tools/genStaticDocs.sh`` (which runs
  ``NatronRenderer`` to dump each node's parameters as reStructuredText).
- Everything else — the User Guide (``guide/``), the Developers Guide
  (``devel/``) and this Maintainer Guide (``maintainers/``) — is written by
  hand.

.. note::

   The master table of contents in ``Documentation/source/index.rst`` normally
   should not be edited, but adding a whole new guide (such as this one) is one
   of the rare cases where it must be: the ``maintainers/index.rst`` entry has to
   be listed in the master ``toctree`` for the guide to appear. Keep such edits
   minimal and deliberate.
