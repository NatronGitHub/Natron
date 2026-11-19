.. _maint-todo:

TODO and Recommended Fixes
==========================

This chapter collects recommendations that emerged from reviewing the code base
for this guide. It is meant as a living backlog of *technical* work — code
health, correctness and modernization — complementary to the user-facing
backlog analyzed in :ref:`maint-issue-triage`. Items are grouped by theme and
tagged with a rough priority (**P0** highest). Where an item maps to a tracked
issue, the issue is linked.

.. contents::
   :local:
   :depth: 1

Toolkit and build modernization
-------------------------------

**Complete Qt 6 support while keeping Qt 5 (P1).**
This is a first-class goal with its own chapter, :ref:`maint-qt6`, which is the
single source of truth for the audited work items and counts. In short: the
removed Qt APIs (``QRegExp``, ``QDesktopWidget``, ``setMargin``) have been
replaced (PR #1084), and Engine, Gui and the Python bindings compile with Qt 6
(PRs #1186–#1188): CMake's ``NATRON_QT6`` generates the Shiboken6/PySide6
bindings, qmake has an untested Qt 6 path without bindings, and
``Global/QtCompat.h`` has the ``QtCompat::MutexLock`` and ``QEnterEvent``
shims. What remains: validate the PySide6 bindings and the GUI at run time
(including the enum/flag issue
`#854 <https://github.com/NatronGitHub/Natron/issues/854>`_), build Qt 6 in CI
(nightly or on demand), and use C++20 with Qt 6 (#1148). Qt 6 work that keeps
Qt 5 unchanged goes to RB-2.6; RB-2.7 is the Qt 6 branch. Tracks
`#1011 <https://github.com/NatronGitHub/Natron/issues/1011>`_ and
`#827 <https://github.com/NatronGitHub/Natron/issues/827>`_.

**Retire the qmake build (P2).**
CMake is the supported build, and the qmake build is deprecated
(`#1181 <https://github.com/NatronGitHub/Natron/issues/1181>`_). The plan:
move the release scripts in ``tools/jenkins`` (Natron and the plug-ins) to
CMake; remove qmake on RB-2.7 first; remove it on RB-2.6 once each installer
built with CMake has been shown to be equivalent to the qmake one. Until then,
keep the ``*.pro``/``*.pri`` files in sync with CMake
(``tools/utils/checkQmakeSources.py`` checks the source lists in CI). The qmake
build gets no new features, in particular no further Qt 6 work.

**Modernize CI (P1).**
Migrate CI from Travis to GitHub Actions
(`#601 <https://github.com/NatronGitHub/Natron/issues/601>`_; the ``.travis.yml``
is already disabled). Build **both** Qt 5 and Qt 6 and run the ``Tests`` suite on
all three platforms so regressions in the dual-toolkit build are caught
automatically.

**Adopt a modern C++ baseline consistently (P3).**
The CMake build already compiles as C++17, and the code has largely moved from
C++98 to ``std::shared_ptr`` etc. Finish the job opportunistically: prefer
``override``, range-based ``for`` (retiring ``foreach``/``Q_FOREACH``, ~46
uses), ``nullptr``, and ``enum class`` where it does not break the Python
bindings. Do this file-by-file alongside other work, not as a mass reformat
(which would fight the astyle hook and bloat diffs).

Stability and correctness (P0)
------------------------------

These are the code-level counterparts of the P0 issues in
:ref:`maint-issue-triage`.

**Make the cache respect its limits (P0).**
`#192 <https://github.com/NatronGitHub/Natron/issues/192>`_ reports that Natron
does not observe disk/RAM cache size limits and crashes when the disk fills.
Audit ``Cache.h`` / ``MemoryFile`` / ``TileCacheFile`` eviction and the
disk-space checks; a compositor that fills the disk and crashes loses work.

**Diagnose the silent render stall (P0).**
`#248 <https://github.com/NatronGitHub/Natron/issues/248>`_ (rendering silently
stalls after N frames) points at the scheduler / abort / thread-pool machinery
(``OutputSchedulerThread``, ``GenericSchedulerThread``, ``AbortableRenderInfo``)
or a deadlock around the cache. High value; reproduce under a debug build with
FP-exception trapping and thread sanitizer.

**Fix launch/teardown crashes (P0).**
Startup crashes (`#845 <https://github.com/NatronGitHub/Natron/issues/845>`_,
`#1008 <https://github.com/NatronGitHub/Natron/issues/1008>`_) and
shutdown/lifecycle crashes (`#795 <https://github.com/NatronGitHub/Natron/issues/795>`_,
`#1029 <https://github.com/NatronGitHub/Natron/issues/1029>`_,
`#1057 <https://github.com/NatronGitHub/Natron/issues/1057>`_) recur across
platforms. Several likely involve OpenGL context creation
(`#516 <https://github.com/NatronGitHub/Natron/issues/516>`_, amdgpu-pro) — a
good place to add defensive checks and clearer diagnostics in ``OSGLContext_*``.

**Invest in the crash-reporting pipeline (P1).**
A large tail of issues are "random crashes" with no stack trace
(`#557 <https://github.com/NatronGitHub/Natron/issues/557>`_). Making the
Breakpad reporter reliably deliver usable minidumps/symbols converts that tail
into fixable, evidence-backed bugs. High leverage.

Robustness of critical subsystems
---------------------------------

**Guard the serialization format (P1).**
Project loading uses Boost.Serialization (see :ref:`maint-design`). There is no
automated test that round-trips and cross-version-loads real project files.
Recommendation: add regression tests that load a corpus of old ``.ntp`` files
and assert they still open; require a ``BOOST_CLASS_VERSION`` bump review for any
``*Serialization`` change. This directly protects users from data loss.

**Replace deprecated/removed Qt string and regex APIs (done).**
``QRegExp`` and the ``QDesktopWidget`` family, the concrete deprecated APIs that
were in use, were replaced in PR #1084 with APIs that exist in Qt 5.15 (details
in :ref:`maint-qt6`).

**Reduce the size of the largest translation units (P3).**
Several files are enormous (``Node.cpp``, ``EffectInstance.cpp`` and
``EffectInstanceRenderRoI.cpp``, ``Knob.cpp``, ``RotoContext.cpp``,
``Settings.cpp``, ``OfxParamInstance.cpp`` — each 100–240 KB). They are hard to
navigate and slow to compile. Where a natural seam exists (as the numbered GUI
files already show), split them into cohesive units. Do this incrementally and
only when already working in the file.

Testing and documentation
-------------------------

**Broaden automated test coverage (P2).**
The ``Tests`` suite covers curves, LUT, images, hashing, tracking and the
file-system model, but not the render pipeline, the cache, serialization, or
knob expressions — the subsystems where the P0 bugs live. Add targeted tests as
these areas are touched; a headless ``NatronRenderer`` render of a known project
compared against a golden image would be a strong end-to-end regression test.

**Keep this Maintainer Guide current (P2).**
Update it when the architecture changes — especially the Qt 6 status in
:ref:`maint-qt6`, and re-run the :ref:`maint-issue-triage` analysis periodically
(the script used to produce it is simple GitHub-API + label aggregation).

**Label and prune the issue tracker (P2).**
As noted in :ref:`maint-issue-triage`, ~half of issues are untriaged and ~124
are 2018 imports that may already be fixed. A labeling/pruning pass makes the
backlog trustworthy and surfaces genuinely actionable work.

Quick-reference: important issues
---------------------------------

Priorities as of the July 2026 snapshot (see :ref:`maint-issue-triage`):

===================  ==================================================  ========
Issue                Summary                                             Priority
===================  ==================================================  ========
`#248`_              Rendering silently stalls after N frames            P0
`#516`_              Won't open on Ubuntu 20.04 (amdgpu-pro)             P0
`#192`_              Cache ignores size limits; crash on full disk        P0
`#845`_ / `#1008`_   Startup crashes (Linux)                              P0
`#795`_ / `#1029`_   Teardown / repeated-open crashes                     P0
`#864`_              CLI renders zero frames                              P0
`#1011`_ / `#854`_   Qt 6 support / Qt flag binding bug                   P1
`#827`_              Qt version tracker                                   P1
`#601`_              Migrate CI to GitHub Actions                         P1
`#215`_ / `#990`_    Ubuntu PPA-Debian repo / Linux binary distrib        P1
`#557`_              Reliable crash reporting                             P1
`#209`_              Viewer 8-bit textures                                P2
`#839`_              Undo broken for on-screen Transform                  P2
`#79`_               Vectorscope & Waveform (top feature)                 P2
===================  ==================================================  ========

.. _#248: https://github.com/NatronGitHub/Natron/issues/248
.. _#516: https://github.com/NatronGitHub/Natron/issues/516
.. _#192: https://github.com/NatronGitHub/Natron/issues/192
.. _#845: https://github.com/NatronGitHub/Natron/issues/845
.. _#1008: https://github.com/NatronGitHub/Natron/issues/1008
.. _#795: https://github.com/NatronGitHub/Natron/issues/795
.. _#1029: https://github.com/NatronGitHub/Natron/issues/1029
.. _#864: https://github.com/NatronGitHub/Natron/issues/864
.. _#1011: https://github.com/NatronGitHub/Natron/issues/1011
.. _#854: https://github.com/NatronGitHub/Natron/issues/854
.. _#827: https://github.com/NatronGitHub/Natron/issues/827
.. _#601: https://github.com/NatronGitHub/Natron/issues/601
.. _#215: https://github.com/NatronGitHub/Natron/issues/215
.. _#990: https://github.com/NatronGitHub/Natron/issues/990
.. _#557: https://github.com/NatronGitHub/Natron/issues/557
.. _#209: https://github.com/NatronGitHub/Natron/issues/209
.. _#839: https://github.com/NatronGitHub/Natron/issues/839
.. _#79: https://github.com/NatronGitHub/Natron/issues/79
