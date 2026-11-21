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

- Each supported release has its own branch, named ``RB-x.y``. **RB-2.6** is
  the current release branch: bug fixes and new features for Natron 2.x go
  there.
- ``master`` holds older development work (last updated in 2021) and is not
  where current work happens. Don't open pull requests against it.
- Fork the repository, make your change on a topic branch, and open a pull
  request against RB-2.6, unless the rules below send it to RB-2.7.

Follow the existing pull-request template
(``.github/PULL_REQUEST_TEMPLATE.md``) and reference the issue(s) your change
addresses.

.. _maint-qt-branches:

Qt 5 and Qt 6 across the release branches
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Natron moves from Qt 5 to Qt 6 in three steps:

- **RB-2.6** (current release) builds with Qt 5, which the releases use, and
  also with Qt 6 (CMake option ``-DNATRON_QT6=ON``). It gets as much Qt 6
  support as possible, **as long as Qt 5 keeps working exactly as before**:
  same behaviour, same build. It is compiled as C++17 with Qt 5 and as C++20
  with Qt 6, so code must compile under both standards.
- **RB-2.7** is the Qt 6 branch, compiled as C++20. While RB-2.6 is
  maintained, RB-2.7 keeps as much Qt 5 support as possible (builds, version
  guards, ``Global/QtCompat.h`` shims), so that changes made on RB-2.6 keep
  merging into it cleanly.
- **RB-2.8** drops Qt 5 entirely: Qt 5 code paths, ``QT_VERSION_CHECK``
  guards, Qt 5 shims and the Qt 5 build configurations go away there.

Where to open a pull request:

- **Bug fixes, features, and Qt 6 changes that leave Qt 5 unchanged:**
  RB-2.6.
- **Qt 5-specific changes** (changes that only make sense with Qt 5, such as
  PySide2 fixes, the MacPorts ``aqua/qt5`` ports, or workarounds for Qt 5
  bugs): RB-2.6 as well. When they reach RB-2.7, the maintainers keep, adapt
  or revert them there.
- **Qt 6 changes that cannot keep Qt 5 working unchanged:** RB-2.7. This is
  the only case where a pull request targets RB-2.7 directly.
- **Changes that only remove Qt 5 support** (dropping guards, shims, or the
  Qt 5 builds): these wait for RB-2.8, which doesn't exist yet. Open an issue
  instead of a pull request.

You don't need to open the same pull request twice: open it on RB-2.6 only.
RB-2.6 is *forward merged* into RB-2.7, as described below.

.. _maint-forward-merge:

Forward-merging RB-2.6 into RB-2.7
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``.github/workflows/forward_merge.yml`` workflow (#1169) runs on every push
to RB-2.6 (and to RB-2.7, or by hand). It keeps one pull request open, "Merge
RB-2.6 into RB-2.7", whose head branch is RB-2.6 itself, so GitHub updates it
with every push. The workflow never pushes anything. Maintainers handle that
pull request as follows:

- Check that the Tests run passes. A pull request opened by a workflow doesn't
  start Tests by itself the first time: close and reopen it.
- Merge it with a **merge commit**, never squash or rebase, so that the next
  forward merge only brings the new commits.
- Then revert or adapt the Qt 5-only changes (PySide2, the MacPorts
  ``aqua/qt5`` ports, Qt 5 workarounds) in a follow-up commit or pull request
  on RB-2.7.
- If the two branches conflict, the workflow opens an issue, "Forward-merge of
  RB-2.6 into RB-2.7 has conflicts", with the commands to run. Resolve the
  conflicts on a separate branch, ``merge-RB-2.6-into-RB-2.7``, made from
  RB-2.7, keeping RB-2.7's Qt 6 code. Open a pull request from that branch to
  RB-2.7 and merge it with a merge commit. Don't use GitHub's "Resolve
  conflicts" button: it would merge RB-2.7 into the head branch, RB-2.6. The
  workflow closes the issue once RB-2.7 contains RB-2.6 or the conflicts are
  gone.

When a pull request mixes these kinds of changes, split it along the same
lines. Smaller pull requests, one topic each, are much easier to review and
test on both Qt versions.

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
unit-test: GUI behaviour can be tested by a GUI test script in ``Tests/gui/``, run by
``Natron --test`` (see :ref:`guiTests`; ``ctest -L gui`` runs them); otherwise, describe your
manual test steps in the pull request.

Two changes deserve special care because they can silently break users:

- **Serialization** (``*Serialization.h``): version every change and keep a
  read path for old projects (see :ref:`maint-design`).
- **The Python API** (``Py*`` classes and typesystems): keep it backward
  compatible (see :ref:`maint-python`).

Testing the GUI from scripts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GUI behavior can also be tested by a Python script run by the Natron GUI with
``--no-dialogs``: the script builds a graph, checks the result, saves screenshots
of Natron's own widgets (``GuiApp.saveScreenshot()``, never a screen capture) and
exits with ``natron.quit(status)``. The Developers Guide chapter :ref:`guiTesting` has a
complete example. ``Tests/gui/run_gui_smoke_test.py`` is the smoke test of the
sources, run by CTest as *GuiSmokeTest*::

    cd build
    OFX_PLUGIN_PATH=/path/to/Plugins OCIO=/path/to/config.ocio ctest -R GuiSmokeTest -V
    # or, keeping the screenshots:
    python3 ../Tests/gui/run_gui_smoke_test.py App/Natron.app/Contents/MacOS/Natron --keep

It is skipped (exit status 77) without a display, in CI unless
``NATRON_GUI_TESTS=1`` is set, or without the openfx-io and openfx-misc
plug-ins. It opens Natron windows (without taking the focus): run GUI tests one at
a time. A crash at exit with some plug-ins (#1253) only prints a warning, unless
``--strict-exit`` is given. ``--debugger`` (or ``NATRON_TEST_WRAPPER=/path/to/wrapper``)
runs Natron under lldb with ``Tests/gui/natron-lldb.sh``: a crash prints the backtraces
of all threads and exits with status 139, without the macOS crash dialog. The same
script can run ``NatronRenderer``, ``natron-python`` or ``Tests``.

Debugging and profiling
-----------------------

The tools for diagnosing a Natron bug are spread across the code base; this is
the consolidated list (several are referenced from the P0 leads in
:ref:`maint-issue-triage`):

- **Debug build.** Build with ``-DCMAKE_BUILD_TYPE=Debug`` (with the
  deprecated qmake build: ``CONFIG+=debug``). This defines ``DEBUG`` and
  enables **floating-point exception trapping** at startup
  (``Global/FloatingPointExceptions.h``), so a stray ``NaN`` or division by
  zero aborts at the source instead of silently polluting the image pipeline.
- **AddressSanitizer.** With CMake, configure with
  ``-DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"``,
  ``-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"`` and
  ``-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address``
  (the deprecated qmake build has ``CONFIG+=addresssanitizer``, see the notes
  at the bottom of ``Project.pro``) — the fastest way into
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

.. _maint-ci:

Continuous integration
----------------------

The CI runs on GitHub Actions, and is kept small on purpose: hosted runners,
macOS ones above all, are scarce, and every related repository (openfx,
openfx-misc, openfx-io, openfx-arena, openfx-gmic, …) follows the same rules.

**Per pull request and per push** (``.github/workflows/ci.yml``, workflow
*Tests*), there are three jobs, each building Natron with CMake and running the
``Tests`` suite:

- **Linux**: Ubuntu 24.04 with its Qt 5, PySide2 and Python 3.12. It also
  checks that qmake and CMake build the same sources, and builds and tests
  twice, in Debug and in RelWithDebInfo.
- **Windows**: MSYS2 UCRT64, with the stock MSYS2 packages (Qt 5, PySide2
  and MSYS2's Python).
- **macOS**: macOS 15 arm64, the system (Xcode) clang and MacPorts libraries
  (never Homebrew), with ``/opt/local`` restored from a cache. It starts only
  after the Linux and Windows jobs have passed.

The ``Tests`` suite includes the GUI tests (``Tests/gui``, ctest label ``gui``, see
:ref:`guiTests`), which take a few seconds each. On Linux they run under Xvfb with Mesa
(the job checks the GLX context first, see #536, and sets ``NATRON_GUI_TESTS=1``),
interactive ones included (``NATRON_TEST_INTERACTIVE=1``), in the Fast (per pull request) and
Release builds, not in the nightly Debug one (``ctest -LE gui``). The environment variable ``NATRON_GUI_TESTS=1`` enables them at run
time; the CMake option ``NATRON_BUILD_GUI_TESTS`` registers them (on by default). In the other jobs, which don't set
``NATRON_GUI_TESTS``, they are skipped (exit status 77), not failed; they can be enabled there
once their runners are known to have an OpenGL context for the viewer.

Linux and Windows must pass before a pull request is merged. The macOS job is
informative: it is never a required check, so a pull request doesn't wait for
it.

The workflow also:

- runs on pushes to ``master``, the ``RB-*`` branches and ``v*`` tags only, and
  on pull requests against ``master`` and ``RB-*``, so that a change on a topic
  branch is built once, through its pull request;
- skips draft pull requests (marking one as ready starts the run);
- ignores changes under ``Documentation/`` (``paths-ignore``): Read the Docs
  builds the documentation;
- cancels a pull request's previous run when it gets a new push
  (``concurrency``), but never a run for a push to a branch or a tag;
- runs with read-only ``permissions`` (``contents: read``).

**Nightly** (03:17 UTC), the macOS job runs alone on the default branch, and is
skipped if the branch didn't change in the last 25 hours. Any branch can also
be run by hand (``workflow_dispatch``). Other configurations (Qt 6, extra C++
standards such as C++23, other compilers or OS versions) are meant to run
nightly, weekly or by hand, never per pull request; none is set up yet.

**Installers and packages are release artifacts.** They are built only by hand
or for pre-release tags, never for pull requests: ``build_installer.yml``
builds the Windows installer and ``build_pacman_repo.yml`` the Windows package
repository, both uploaded as workflow artifacts. The macOS and Linux installers
are built outside GitHub Actions: the macOS release builds with ``tools/tart``
(in a macOS VM on a maintainer's Apple Silicon Mac), the Linux installers with
the ``tools/jenkins`` scripts in the Rocky Linux 9 SDK Docker image (being
adapted from ``tools/docker/natron-sdk-rockylinux8``, see `#1343
<https://github.com/NatronGitHub/Natron/pull/1343>`_).

Where to start
--------------

The project README grades starter tasks by difficulty. A reasonable on-ramp is:
build Natron locally; fix a small, well-scoped bug labeled ``difficulty:easy``;
then take on a ``difficulty:medium`` item. The :ref:`maint-issue-triage` chapter
lists concrete good-first-issue candidates.

Building this manual
--------------------

The documentation is `Sphinx <https://www.sphinx-doc.org/>`_ using the Read the
Docs theme; sources are under ``Documentation/source``. Install the Python
requirements in a virtual environment, then build the HTML with::

    pip install -r Documentation/source/requirements.txt
    cd Documentation
    sphinx-build -b html source html

and the PDF with::

    sphinx-build -b latex source pdf
    (cd pdf; latexmk -pdf Natron)

The Developers Guide includes the OpenFX documentation of the ``libs/OpenFX``
submodule (``Documentation/source/_ext/natron_openfx.py`` copies it into
``devel/openfx/``, which git ignores). It needs the submodule
(``git submodule update --init libs/OpenFX``), breathe and Doxygen; without
them, or with ``NATRON_DOCS_OPENFX=0`` (which the installers set, to keep the
documentation bundled with Natron small), those pages are placeholders.

Use Doxygen 1.17 or earlier (Read the Docs uses the Doxygen of Ubuntu 22.04,
1.9.1). Doxygen 1.18 writes the suite function pointers in a form that breathe
cannot parse, so the reference would lose those declarations, with about 330
warnings: with Doxygen 1.18 or later, ``natron_openfx.py`` writes the
placeholders instead and says so in the build log (look for
``natron_openfx:``). ``NATRON_DOCS_OPENFX=force`` builds the OpenFX pages
anyway, e.g. to check a change to them; MacPorts and Homebrew ship Doxygen
1.18 or later.

Read the Docs builds HTML, PDF and ePub for each push to a release branch
(see ``.readthedocs.yaml``), but only the HTML for a pull request: check the
PDF and ePub locally when a change can affect them.

Diagrams
~~~~~~~~

Write diagrams as `Mermaid <https://mermaid.js.org/>`_ text in a ``mermaid``
directive (from `sphinxcontrib-mermaid
<https://github.com/mgaitan/sphinxcontrib-mermaid>`_), not as image files, so
that they can be reviewed and updated like the rest of the text. For example::

    .. mermaid::
       :alt: Gui links Engine, Engine links HostSupport.
       :caption: What links what.

       flowchart TB
           Gui["Gui/"] --> Engine["Engine/"]
           Engine --> HostSupport["HostSupport/"]

- Use the diagram types of `Mermaid 11 <https://mermaid.js.org/intro/>`_
  (``flowchart``, ``sequenceDiagram``, ``classDiagram``, ``stateDiagram-v2``,
  …), and try them in the `Mermaid live editor <https://mermaid.live/>`_.
- Always give an ``:alt:`` text that says what the diagram shows, and keep
  diagrams small: a diagram complements the text, it does not replace it.
- Quote labels that contain punctuation (``A["Engine/ (NatronEngine)"]``);
  ``<br/>`` breaks a line.

The pictures come from the Node.js packages of ``Documentation/package.json``
(versions locked in ``package-lock.json``). Install them once, with Node.js 20
or later::

    cd Documentation
    npm ci    # node_modules/ is ignored by git

The HTML pages draw the diagrams in the browser, with the mermaid.js of
``node_modules`` (copied to ``_static/mermaid/``, so that a local or offline
build works without a network connection). ``tools/jenkins/gen-natron-doc.sh``,
which builds the documentation bundled with Natron, does not run ``npm ci``:
unless ``Documentation/node_modules`` already exists there, the bundled
documentation shows the diagrams as source. The PDF and ePub need pictures,
which `mermaid-cli <https://github.com/mermaid-js/mermaid-cli>`_ draws with a
headless Chrome; ``_ext/natron_mermaid.py`` checks that it works before the PDF
or ePub build. Without sphinxcontrib-mermaid or ``node_modules``, or if
mermaid-cli doesn't work, the diagrams are shown as source, and the build log
says why (look for ``mermaid:``).

Two rules from ``Documentation/README-Natron-documentation.md`` are important:

- **Do not hand-edit** ``index.rst``, files whose names start with ``_``, or
  anything under ``plugins/``. The per-node pages in ``plugins/`` are generated
  from the plug-ins themselves with ``tools/genStaticDocs.sh`` (which runs
  ``NatronRenderer`` to dump each node's parameters as reStructuredText).
  The same run generates the Reference Guide page (``_group.rst``), the
  "Nodes" page (``_nodes.rst``), and one ``_group<id>.rst`` page per node group
  and sub-group. Each level of a plug-in's OFX grouping is a group (empty
  levels are skipped, and a plug-in without a grouping goes to "Other"). The
  ``<id>`` keeps the ASCII letters and digits of each level, drops the other
  ASCII characters, encodes non-ASCII characters as ``xHHHH``, and joins the
  levels with ``_`` (``G'MIC/Colors`` gives ``_groupGMIC_Colors.rst``); an id
  that differs from an existing one only by case gets a ``-2`` suffix. See
  ``Engine/DocumentationGroups.h``; the in-app documentation server
  (``Gui/DocumentationManager.cpp``) builds the same pages on the fly.
- Everything else — the User Guide (``guide/``), the Developers Guide
  (``devel/``) and this Maintainer Guide (``maintainers/``) — is written by
  hand.

.. note::

   The master table of contents in ``Documentation/source/index.rst`` normally
   should not be edited, but adding a whole new guide (such as this one) is one
   of the rare cases where it must be: the ``maintainers/index.rst`` entry has to
   be listed in the master ``toctree`` for the guide to appear. Keep such edits
   minimal and deliberate.
