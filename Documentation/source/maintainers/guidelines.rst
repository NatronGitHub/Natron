.. _maint-guidelines:

Project Guidelines
==================

These are the working rules of the Natron maintainers, for Natron and for the
OpenFX repositories built with it: `openfx
<https://github.com/NatronGitHub/openfx>`_, `openfx-supportext
<https://github.com/NatronGitHub/openfx-supportext>`_, `openfx-misc
<https://github.com/NatronGitHub/openfx-misc>`_, `openfx-io
<https://github.com/NatronGitHub/openfx-io>`_, `openfx-arena
<https://github.com/NatronGitHub/openfx-arena>`_ and `openfx-gmic
<https://github.com/NatronGitHub/openfx-gmic>`_. Unless a rule says otherwise,
it applies to all of these repositories; `Differences between the
repositories`_ lists the exceptions. Other chapters have the details:
:doc:`contributing` (branches, tests, the CHANGELOG), :doc:`github-builds`
(the CI and the packages) and :doc:`issue-triage`.

The project has a single maintainer, so most of these rules exist to save
review time and CI runs, and to keep the release branches mergeable.

.. contents::
   :local:
   :depth: 1

Branches, Qt 5 and Qt 6 (Natron)
--------------------------------

- **RB-2.6 is the release branch** (bug fixes and features). ``master`` holds
  older work and is not used. *Why: one branch to review and release.*
- **Qt 5 must keep working 100% on RB-2.6**: same behaviour, same build. Add
  as much Qt 6 support there as possible; a change that adds Qt 6 support but
  breaks, degrades or removes anything under Qt 5 goes to RB-2.7.
- **RB-2.7 is the Qt 6 branch.** While RB-2.6 is maintained, it keeps as much
  Qt 5 support as possible (builds, guards, shims), so that RB-2.6 changes keep
  merging into it. **RB-2.8** drops Qt 5 entirely.
- **Open pull requests on RB-2.6 only**; open one on RB-2.7 only for Qt 6 work
  that can't keep Qt 5 working. Review Qt 6 pull requests commit by commit
  against these rules, and split large mixed ones.
- **Forward merge:** every RB-2.6 change that isn't Qt 5-specific must reach
  RB-2.7. A workflow keeps one "Merge RB-2.6 into RB-2.7" pull request open;
  merge it with a merge commit (never squash, never rebase it), after
  reverting or adapting the Qt 5-only changes (PySide2, the MacPorts
  ``aqua/qt5`` ports, Qt 5 workarounds). Resolve conflicts on a
  ``merge-RB-2.6-into-RB-2.7`` branch made from RB-2.7, keeping RB-2.7's Qt 6
  code, never with GitHub's "Resolve conflicts" button. See
  :ref:`maint-forward-merge`.
- **Qt 6 builds are verified on RB-2.7.** Fix the problems that RB-2.7's Linux
  Qt 6 CI finds on RB-2.6 (keeping Qt 5 working), then forward-merge again.
  Don't add Qt 6 CI jobs to RB-2.6, and build Qt 6 locally only on RB-2.7.
- **Every Qt 5-only section needs its Qt 6 counterpart**, and the other way
  round: a change in code under ``QT_VERSION < QT_VERSION_CHECK(6, 0, 0)`` (or
  the Qt 5 side of an ``#if``/``#else``), CMake code under ``NOT NATRON_QT6``,
  qmake-only settings, PySide2-only code, typesystem entries or patches, a Qt
  5-only CI job or an ``aqua/qt5`` port comes, in the same commit, with the
  equivalent change on the Qt 6 side, or with a hard failure there
  (``#error "TODO(Qt6): port <commit> <summary>"``; in CMake,
  ``message(FATAL_ERROR "TODO(Qt6): …")``). Reviewers check every
  Qt-version-conditional hunk. Exception: a Qt 5-only bug fix whose bug the Qt 6
  code path doesn't have; say so in the commit message. *Why: otherwise the Qt
  6 side silently misses fixes.*

Build system: CMake replaces qmake
----------------------------------

- **Everything moves to CMake** (#1181): every build, CI job and installer
  script, for Natron and the plug-ins. qmake is removed on RB-2.7 first.
- **On RB-2.6, qmake stays** (``NATRON_BUILD_SYSTEM=qmake`` in
  ``tools/jenkins``; CMake is the default) until each CMake-built installer has
  been shown to be equivalent. Until then, **keep the source lists in sync**
  between the ``*.pro``/``*.pri`` files and ``CMakeLists.txt``; CI checks it
  (``tools/utils/checkQmakeSources.py``).
- The plug-ins' Makefiles stay as fallbacks of Natron RB-2.6's release
  scripts; their CI builds with CMake, and their weekly runs check that the
  Makefiles still build.

Code (Natron)
-------------

- **Engine never includes Gui headers**: the headless ``NatronRenderer`` links
  Engine alone. Engine talks to Gui through the ``*I.h`` interfaces
  (``NodeGuiI``, ``KnobGuiI``, …).
- **Code that runs during a render reads the captured per-render state**
  (``ParallelRenderArgs``), not the live node state. *Why: the user may change
  the node while it renders.*
- Knob signals go through ``KnobSignalSlotHandler``.
- All code inside ``NATRON_NAMESPACE_ENTER``/``NATRON_NAMESPACE_EXIT``;
  ``<Python.h>`` first in every translation unit; ``QT_NO_CAST_FROM_ASCII``
  (use ``QString::fromUtf8()`` or ``tr()``); parent-to-child ``shared_ptr``,
  child-to-parent ``weak_ptr``; new types registered in the ``*Fwd.h``
  headers. See :doc:`design-techniques`.
- **Keep saved projects and scripts working.** The ``*Serialization`` classes
  and the ``Py*`` Python API are backward-compatibility-critical: bump
  ``BOOST_CLASS_VERSION`` on a serialization change, and keep the Python API
  stable (see :doc:`python-bindings`). For the plug-ins, see `Differences
  between the repositories`_.
- **Qt 5 and Qt 6 must both build**: prefer APIs that both have, then the
  shims of ``Global/QtCompat.h``, then ``QT_VERSION_CHECK`` guards.
- **Never silence deprecation warnings** (no pragmas, ``-Wno-deprecated*`` or
  ``QT_NO_DEPRECATED_WARNINGS``): port the code to the replacement API, or
  leave the warning visible and open an issue with the migration plan
  (behaviour differences, call sites, Qt version). *Why: a silenced warning
  becomes a build break at the next major version.*
- **Style:** astyle, enforced by ``.git-hooks/pre-commit`` (see
  :doc:`building`).
- **Docs:** don't hand-edit the generated ``_*.rst`` files, ``plugins/``, or
  ``index.rst`` (except to add a guide to its table of contents).
- **OpenFX:** check the `OpenFX specification
  <https://openfx.readthedocs.io>`_ (the headers and reference of
  `AcademySoftwareFoundation/openfx
  <https://github.com/AcademySoftwareFoundation/openfx>`_) before writing or
  reviewing code that touches the OFX API, not what a host happens to do, and
  quote it when a decision depends on it. The OFX ABI is fixed.

Review standards
----------------

Review the way Bjarne Stroustrup and Herb Sutter would.

- **C++:** against the `C++ Core Guidelines
  <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines>`_, citing rule
  IDs, each linked to its section, e.g. `R.3
  <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ptr>`_,
  `F.20 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-out>`_,
  `C.21 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-five>`_,
  `CP.42 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-wait>`_,
  `ES.48 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts>`_,
  `I.4 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-typed>`_.
- **Python:** our own Python (tests, PyPlugs, ``tools/`` scripts, examples in
  the docs; not generated or third-party code) against the `Google Python
  Style Guide <https://google.github.io/styleguide/pyguide.html>`_, citing
  section names, e.g. `3.8 Comments and Docstrings
  <https://google.github.io/styleguide/pyguide.html#s3.8-comments-and-docstrings>`_.
- **Link every citation to its exact section.** Take the anchors from the
  guides' own headings (the ``<a name="…">`` of ``CppCoreGuidelines.md``, the
  ``#s…`` ids of ``pyguide.html``), lowercase as on the published page; never
  guess them.
- **Check:** correctness; ownership and lifetimes; exception safety; thread
  safety (which data each mutex guards, lock order, waits with a predicate, the
  GIL); interfaces (no bool soup, no out-parameters where a return value
  works); duplication; data structures; readability (function size, nesting);
  documentation.
- **Review the test results, not only the diff:** skips, expected failures,
  warnings, runtimes that look wrong.
- **Scope:** new and changed code must follow these rules; code only touched in
  passing is not a blocker, but note the improvements with file:line.
- **Constraints win:** Qt 5 and Qt 6 both build, the C++ standards below, the
  fixed OFX ABI, the ``*Serialization`` format and the Python API. When a
  guideline gives way to one of them, say so.
- **No over-engineering.** A guideline citation alone doesn't justify work.
  Keep real bugs, crashes, hangs, leaks, spec violations and cheap safety fixes;
  don't ask for churn in stable code, unmeasured optimisations, cosmetic
  changes, or new API and test machinery that nobody needs.

C++ and Python versions
-----------------------

- **C++ on RB-2.6:** C++17 for Qt 5 builds, C++20 for Qt 6 builds, for Natron
  and every plug-in built with it. Code must compile as both; the build scripts
  pick the standard from the Qt major version. **RB-2.7:** C++20 everywhere.
- **Python:** Qt 5 builds use Python 3.11 with Qt 5.15.19 and
  PySide2/Shiboken2 5.15.19; Qt 6 builds use Python 3.13 (VFX Reference
  Platform CY2026/CY2027) with PySide6. Python 3.10 (end of life) is not used.
  Exceptions: Windows uses MSYS2 UCRT64's rolling Python and its PySide2; the
  Linux CI uses Ubuntu 24.04's Python 3.12 and PySide2; RB-2.7's Linux Qt 6 CI
  uses Ubuntu 26.04's Python 3.14 and PySide6 6.10.

CI and packages
---------------

The workflows are described in :doc:`github-builds`; the rules are:

- **Exactly three full builds with tests per pull request:** macOS 15 arm64
  (the system Xcode clang and MacPorts libraries), one Linux target (Ubuntu
  24.04 today) and one Windows target (MSYS2 UCRT64 for Natron).
- **macOS is informative.** macOS runners are scarce: the macOS job runs only
  after Linux and Windows pass, and it is never a required check. Don't re-run
  a macOS job that was cancelled or got no runner.
- **Green CI means Linux and Windows passed**, judged by the latest run of each
  job. A Windows job that didn't run for lack of a runner (cancelled or timed
  out before it started, or still queued after 3 hours) counts as green if
  Linux passed; one that ran and failed is red.
- **Rely on GitHub CI, not local builds.** Build or test locally only what CI
  can't (GUI tests, plug-ins or Natron-Tests that CI lacks, crash debugging,
  SDK, MacPorts or installer work, a CI failure the log doesn't explain), and
  never only for nits or re-reviews. Push a test together with its fix.
- **Everything else is weekly, never nightly,** or run by hand: extra C++
  standards, Qt 6, Debug builds, other compilers and OS versions. The plug-in
  repositories and openfx run on Saturday (03:17 UTC), Natron on Sunday.
  Natron's weekly run builds all three OSes and is never skipped. In the other
  repositories, a weekly run may be skipped only if the default branch's head
  is the commit that the last scheduled run (passed or failed, not cancelled)
  tested, and never for jobs that build against a moving upstream or a rolling
  toolchain.
- **Workflow hygiene:** ``paths-ignore`` for documentation, ``concurrency``
  with ``cancel-in-progress`` for pull requests, read-only ``permissions``, one
  dependency cache per OS. User-facing artifacts are kept at least 14 days.
- **Pre-release packages** (macOS DMG, Linux portable archive, Windows zip with
  Setup.exe) are built on demand or on pre-release tags, never on pull
  requests. Only Natron attaches them to a GitHub pre-release; the plug-in
  repositories only upload CI artifacts. Each Natron pre-release tag is also
  created on the plug-in repositories, and every package workflow builds the
  plug-ins at that tag (not yet on macOS and Windows, #2051; see
  :doc:`github-builds`).
- **macOS deployment target:** CI builds, which aren't shipped, target the
  runner's macOS; only the release builds target macOS 14.0. Natron 2.6
  supports macOS 14 and later, arm64 and x86_64.
- **macOS compiler:** the system (Xcode or Command Line Tools) clang, when it
  supports the needed standard and has no blocking bug that a newer clang
  fixes; a MacPorts clang only otherwise (#1134).
- **Windows toolchains:** Natron, openfx-io and openfx-arena are never built
  with MSVC: MSYS2 UCRT64, and MSYS2 CLANGARM64 for native ARM64 (#1494).
  openfx, openfx-misc (without OSMesa) and openfx-gmic may use MSVC or UCRT64
  per pull request (one fast C++17 build); their weekly runs use MSVC for
  x86-64 and Arm64X (or, if Arm64X isn't possible, Arm64EC and AArch64).
- **No COMMERCIAL builds** (``NATRON_LICENSE`` or ``OFX_LICENSE`` set to
  ``COMMERCIAL``) in CI, weekly runs or releases; scripts may keep a manual
  option. COMMERCIAL work is labelled ``commercial`` and ``prio:low``. GPL
  builds never drop their GPLv3 components.
- **No crash reporter in 2.6 releases** (no Breakpad); one comes back only with
  Sentry.

Pull requests
-------------

The process, for every pull request, code or documentation:

1. **Open it as a draft.** Drafts run no CI. Push the test and the fix
   together.
2. **Two independent reviewers** review the draft. The author fixes the
   findings and the reviewers review again, until both say "merge" (see
   `Reviews`_).
3. **Rebase it on the latest base branch** (RB-2.6 for Natron, ``master``
   for the openfx repositories) and force-push with lease (the pull request's
   branch only). Run CI on the rebased head (by hand, since drafts run none,
   see :doc:`github-builds`), and mark it ready once that run passes; marking
   it ready runs CI again. Never rebase the RB-2.6 to RB-2.7 forward merge.
4. **CI passes:** the maintainer reviews it. **Only after the maintainer's
   approval** is it merged, and only when the reviewers say "merge" on the
   final head, every finding they raised is fixed or answered, CI passes on
   that head, and it is rebased on the latest base branch. A pull request that
   changes after the approval needs the reviewers' re-check, not a new
   approval. Never enable auto-merge.
5. **CI fails:** put it back to draft, fix it, and go back to step 2.

Merge methods (Natron): squash a pull request that fixes one issue; rebase (one
commit per issue) a pull request that fixes several; a merge commit for the
forward merge; rebase or a merge commit (never squash) for the CHANGELOG
collection, whose commit message the next collection reads (see :ref:`maint-changelog`).

**Fewer pull requests, more issues fixed in each.** Group related fixes (same
area, same files, or the same kind of change) into one pull request, one
commit per issue, each with its test. Open a separate pull request only for
unrelated areas, for a fix that must merge on its own, or for an Engine change.
*Why: each pull request costs a review, a rebase and a CI run.*

**Engine changes get their own issue and pull request.** A pull request for a
script or a tool contains only the tool, its installation, documentation and
tests; the Engine and Python API changes it needs go into a separate pull
request, and the tool's pull request is stacked on it. *Why: Engine changes
need a closer review, and must not wait for the tool.*

**Labels:** every pull request carries the labels of the issues it fixes or is
part of, ``prio:`` and ``difficulty:`` included (with several issues, the
highest priority and the hardest difficulty). A pull request without an issue
gets its own ``prio:``, ``difficulty:``, ``type:`` and ``func:`` labels.

Descriptions and the CHANGELOG
------------------------------

- **Write for one maintainer reading it cold.** A pull request description
  (and an issue body, and a review comment) is short and self-contained: the
  problem in terms of what users see, what changes, what was tested, and what
  the maintainer must decide or check. Link issues instead of restating them;
  no outside context ("as discussed"). Everything else (file:line details,
  alternatives, full test matrices, guideline citations, review rounds) goes
  in a collapsed ``<details>`` block.
- **Pull requests don't edit CHANGELOG.md.** The description ends with a
  ``### CHANGELOG`` section: its lines, each under its target heading (``New
  features``, ``Changes``, …), or ``none``. A script collects the lines of the
  pull requests merged since the last collection into ``CHANGELOG.md``, in one
  commit, weekly and always before a pre-release tag, and each collection
  regroups and rewrites the changelog for users. *Why: every pull request that
  edited CHANGELOG.md conflicted with all the others.* The format and the
  procedure are in :ref:`maint-changelog`.

Reviews
-------

- **Each review round is posted on the pull request**, as a GitHub review with
  state *Comment* (the maintainer's account can't approve its own pull
  requests): one short summary per reviewer and round ("Reviewer A, round N,
  at <sha>: merge / merge after fixes / changes needed", at most three lines of
  blocking findings, the rest in ``<details>`` with the guideline links), and
  comments on the lines each finding is about.
- **The author answers each finding** with the commit that fixes it, or the
  reason it stays as is, and **resolves the thread** as it processes it, so
  that only the open findings stay unresolved. A reviewer who disagrees with an
  answer reopens the thread and replies in it.
- **No duplicate line comments.** Before posting a line comment, read the
  comments already on the pull request. If one on the same lines makes the
  finding, reply in that thread ("Reviewer B: agreed; also …") instead, or edit
  your own earlier comment when your finding changed. A re-review round reuses
  its earlier threads.
- **Reconcile after each round.** Reviewers review independently and in
  parallel; then each reads the other reviewers' reviews of that round.
  Duplicate threads on the same finding are merged (the later one becomes a
  one-line "Same as …" pointer, and the author answers once). In the others'
  threads, each reviewer says briefly whether it agrees, disagrees (with a
  reason) or has something to add, and may then change its verdict. The round
  ends when every reviewer has done this.
- **Week-review pull requests get a third reviewer.** A week-review pull
  request is a cleanup from reviewing a past period's code (duplication,
  over-engineering, clarity, readability), with the bugs found on the way.
  Reviewer C reviews correctness only, commit by commit, after reading the
  other reviews: every refactor keeps the behaviour exactly (arguments,
  defaults, edge cases, which copy won where copies had drifted, errors, side
  effects, threads and locks, call order), each bug fix is right and complete,
  no test is weakened, and API, ABI, serialization and build configuration
  stay compatible. Such a pull request merges only when all three reviewers
  say "merge".
- **External pull requests and issues get two independent reviewers too.**
  For an issue: is the bug real and reproduced, what is the root cause, its
  priority, and does it block a release. Their combined findings go to the
  contributor as one concise comment, posted once the maintainer approves it;
  a pull request that needs changes goes back to draft until it is updated.

Issues
------

- **Every open issue has one** ``prio:{high,normal,low}`` **and one**
  ``difficulty:{easy,medium,hard}`` **label**, each set balanced to about a third
  of the open issues. Priority follows the impact on users: crashes, data
  loss, wrong output and hangs first, then broken features, release needs,
  performance, documentation and cleanup. Release blockers (milestone 2.6) are
  always ``prio:high``; COMMERCIAL-build issues are ``prio:low``.
- **Work by descending priority.** Triage all open issues again (both labels)
  whenever ``prio:high`` (the important ones) or ``difficulty:easy`` (the
  low-hanging fruit) becomes empty. Triage changes labels only.
- See :doc:`issue-triage` for Natron's open issues.

Natron-Tests known failures
---------------------------

- **Most known failures go in the common list**; a platform or architecture
  list only gets a test that a run shows passing elsewhere. *Why: known
  failures should hardly differ between platforms; when they do, that points
  at a real bug.*
- **Every entry cites an open Natron issue**, and a known failure that passes
  fails the run. How the lists work: :doc:`github-builds`, "Natron-Tests".

macOS: MacPorts and INSTALL_MACOS.md
------------------------------------

- **MacPorts, never Homebrew**, for every macOS build: a Homebrew tool on the
  ``PATH`` silently pulls in Homebrew headers and libraries.
- **Keep the overlay's three files consistent on every Portfile change**
  (``tools/MacPorts``): ``Portfile`` (what MacPorts uses), ``Portfile.orig``
  (the upstream Portfile it is based on) and ``Portfile.patch`` (Natron's
  changes, ``diff -u Portfile.orig Portfile``); overlay-only ports have only a
  ``Portfile``, and pinned ports (with a ``PINNED`` file) keep their own
  ``Portfile`` while ``Portfile.orig`` follows upstream. Then run
  ``./sync.sh`` and check ``Ports failed: 0``. *Why:* so that ``check.sh``
  prints upgrade commands that work. See :doc:`build-macos`.
- **INSTALL_MACOS.md is the single from-scratch macOS reference.** Update it in
  the same pull request whenever a change affects the install (ports or
  variants, pins, compiler, OSMesa/LLVM, Python/Qt, build commands), or when
  running the steps shows a missing, wrong or out-of-order step, and keep
  ``tools/tart/provision.sh`` consistent with it.

Differences between the repositories
------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Topic
     - Difference
   * - Base branch
     - Natron: RB-2.6 (RB-2.7 for Qt 6-only work). The openfx repositories:
       ``master``.
   * - Windows and weekly runs
     - See `CI and packages`_.
   * - Merge method
     - Natron: squash for one issue, rebase for several. openfx-io: one commit
       per pull request (review fixes squashed). openfx-arena: squash.
   * - CHANGELOG
     - Every pull request has a ``### CHANGELOG`` section. Natron's collection
       also reads those of the plug-in repositories (see
       :ref:`maint-changelog`). openfx-gmic's ``CHANGELOG.md`` and
       openfx-arena's ``ChangeLog.md`` are updated when a pre-release is
       tagged, with the lines collected from their pull requests; pull
       requests don't edit them. openfx-io, openfx-misc and openfx-supportext
       have no changelog file.
   * - Draft CI
     - openfx runs its bindings-sync check on drafts: a change to a virtual
       function of ``HostSupport/include`` updates the Python adapters of
       ``HostSupport/python/src``, or says why not in a ``Bindings:`` line.
   * - Saved projects
     - The plug-ins: a plug-in gets a new major version only if its current
       major shipped in Natron 2.5.0, the change is incompatible (a project
       saved with it could no longer be loaded: a parameter removed, renamed or
       retyped, choice options reordered) and the behaviour differs; the old
       major then stays registered and unchanged. A bug fix needs no new major,
       even when it changes renders, and new choice options go at the end.
       openfx-supportext: a fix that changes plausible output of existing
       projects keeps the old behaviour behind a flag that the plug-in enables
       for its old version (crash, NaN and garbage fixes excepted). Library
       upgrades in 2.6 (e.g. ImageMagick 7, a new G'MIC) may change the output;
       old results need Natron 2.5.
   * - C++ standard
     - openfx Support and HostSupport, and openfx-supportext: C++17 minimum;
       their users build them as C++17 to C++23.
