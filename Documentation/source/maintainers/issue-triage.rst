.. _maint-issue-triage:

Open Issue Triage
=================

This chapter categorizes and prioritizes the project's open issues on the
`GitHub tracker <https://github.com/NatronGitHub/Natron/issues>`_ to help
maintainers decide what to work on. It is a **snapshot** (271 open issues at the
time of writing) and a *method*, not a live list — re-run the analysis
periodically. Issue numbers link to GitHub.

.. note::

   Snapshot generated in July 2026 against the ``NatronGitHub/Natron`` tracker.
   The absolute counts below will drift; the categories and prioritization
   method are the durable part. Re-generate with the GitHub issues API plus
   label aggregation (a few lines of Python) when you need fresh numbers.

How the tracker is labeled
--------------------------

Issues are (mostly) labeled along several axes, which makes triage tractable:

- **Type** — ``type:bug`` (99), ``type:feature`` (143), ``type:task``,
  ``type:support``, ``type:idea``.
- **Functional area** — ``func:gui`` (54), ``func:engine`` (23),
  ``func:plugins`` (22), ``func:viewer`` (18), ``func:nodegraph`` (17),
  ``func:roto`` (15), ``func:readers`` (14), ``func:writers`` (8),
  ``func:python`` (8), ``func:buildsystem`` (7), ``func:tracker`` (6),
  ``func:crashreport`` (1).
- **Priority** — ``prio:high`` (4), ``prio:normal`` (24), ``prio:low`` (25).
- **Difficulty** — ``difficulty:easy`` (10), ``difficulty:medium`` (11),
  ``difficulty:hard`` (3).
- **Status** — ``status:confirmed`` (21), ``status:in-progress`` (5),
  ``status:fix-commited`` (1).

Note that the large majority of issues have **no priority or difficulty label**;
part of good maintenance is labeling them as they are reviewed. About 124 issues
date from a 2018 bulk import from the previous tracker and should be re-validated
against current releases — many may already be fixed.

Prioritization framework
------------------------

Rank work by user impact, using this order:

- **P0 — Stability & data integrity.** Crashes, failures to launch, failures to
  render, hangs, and anything that loses a user's work. These erode trust the
  fastest and should be fixed first.
- **P1 — Sustainability.** Keeping Natron buildable, packaged and current: the
  Qt 6 migration, CI, and binary distribution. Without these the project cannot
  ship fixes at all.
- **P2 — High-demand functional bugs & features.** Confirmed bugs and
  frequently-requested features in core workflows (viewer, roto, tracker,
  color).
- **P3 — Polish & niche features.** Low-priority UX and specialized requests.

Within a tier, weight by engagement (comments + reactions) and by whether a fix
is confirmed/scoped. A simple engagement score of ``comments + 2×reactions`` is
used below to surface what users actually care about.

P0 — Stability (fix first)
--------------------------

Crashes and render failures dominate real user pain. These should be the top of
the backlog:

- `#248 <https://github.com/NatronGitHub/Natron/issues/248>`_ — *Rendering
  silently stalls after X frames* (engine; very high engagement;
  ``info:help-needed``). A silent render stall is catastrophic for the core use
  case.
- `#516 <https://github.com/NatronGitHub/Natron/issues/516>`_ — *Won't open on
  Ubuntu 20.04 with amdgpu-pro drivers* (confirmed, engine+gui, hard). A
  cannot-launch bug for a whole class of users; likely an OpenGL context issue
  (see ``OSGLContext_*``).
- `#845 <https://github.com/NatronGitHub/Natron/issues/845>`_,
  `#1008 <https://github.com/NatronGitHub/Natron/issues/1008>`_,
  `#907 <https://github.com/NatronGitHub/Natron/issues/907>`_ — *crashes on
  startup* (several reports, various Linux). Startup crashes block everything.
- `#795 <https://github.com/NatronGitHub/Natron/issues/795>`_ — *stuck in memory
  when closing a project*; `#1029 <https://github.com/NatronGitHub/Natron/issues/1029>`_
  — *crash after repeatedly opening/closing*; `#1057
  <https://github.com/NatronGitHub/Natron/issues/1057>`_ — *crash closing the
  file picker for Writer*. Lifecycle/teardown bugs.
- `#192 <https://github.com/NatronGitHub/Natron/issues/192>`_ — *does not
  observe cache size limits, crashes when disk full* (engine cache). Ties into
  the ``Cache`` subsystem.
- `#864 <https://github.com/NatronGitHub/Natron/issues/864>`_ — *command line
  renders zero frames only* (``prio:high``); `#1047
  <https://github.com/NatronGitHub/Natron/issues/1047>`_ — *ReadFFmpeg
  performance* (``prio:high``, ``func:readers``).

A recurring theme (``#557``, ``#862``, ``#903``, ``#958``, ``#1005``,
``#1068`` …) is *random / hard-to-reproduce crashes*. Investing in the crash
reporter pipeline (``#557``) so real stack traces come back from users would pay
off across all of these.

P1 — Sustainability (build, Qt 6, distribution)
-----------------------------------------------

- **Qt 6 migration** — `#1011 <https://github.com/NatronGitHub/Natron/issues/1011>`_
  (QT6 support), `#854 <https://github.com/NatronGitHub/Natron/issues/854>`_
  (Qt flag/enum binding bug), tracked historically alongside the Qt5 tracker
  `#827 <https://github.com/NatronGitHub/Natron/issues/827>`_. See
  :ref:`maint-qt6` for the concrete plan.
- **CI modernization** — `#601
  <https://github.com/NatronGitHub/Natron/issues/601>`_ (migrate CI from Travis
  to GitHub Actions; ``difficulty:easy``). A prerequisite for reliably shipping
  everything else, and a good first task.
- **Binary distribution** — `#990
  <https://github.com/NatronGitHub/Natron/issues/990>`_ (Linux 2.6 distrib,
  ``help-needed``), `#741 <https://github.com/NatronGitHub/Natron/issues/741>`_
  / `#743 <https://github.com/NatronGitHub/Natron/issues/743>`_ (Linux/Windows
  Qt5 distrib), `#215 <https://github.com/NatronGitHub/Natron/issues/215>`_
  (Ubuntu PPA / Debian repo — the single most-commented issue),
  `#318 <https://github.com/NatronGitHub/Natron/issues/318>`_ (AppImage),
  `#681 <https://github.com/NatronGitHub/Natron/issues/681>`_ (update Windows
  SDK). `#672 <https://github.com/NatronGitHub/Natron/issues/672>`_ (downgrade
  macOS libomp; ``difficulty:easy``) and `#668
  <https://github.com/NatronGitHub/Natron/issues/668>`_ (FreeBSD) round this out.

P2 — Confirmed functional bugs & popular features
-------------------------------------------------

Confirmed bugs in core workflows (good, scoped targets):

- `#209 <https://github.com/NatronGitHub/Natron/issues/209>`_ — Viewer 8-bit
  textures don't work (viewer).
- `#206 <https://github.com/NatronGitHub/Natron/issues/206>`_,
  `#367 <https://github.com/NatronGitHub/Natron/issues/367>`_,
  `#510 <https://github.com/NatronGitHub/Natron/issues/510>`_,
  `#178 <https://github.com/NatronGitHub/Natron/issues/178>`_ — roto/rotopaint
  color-picking, mask input, first-frame stroke, bounding box.
- `#839 <https://github.com/NatronGitHub/Natron/issues/839>`_ — undo broken for
  on-screen Transform (``prio:high``, confirmed).
- `#195 <https://github.com/NatronGitHub/Natron/issues/195>`_ — conflicting
  roto/tracker key bindings (fix committed — verify & close).
- `#191 <https://github.com/NatronGitHub/Natron/issues/191>`_ — reader/writer
  breaks when the file extension changes.

Most-requested features (weight by demand):

- `#79 <https://github.com/NatronGitHub/Natron/issues/79>`_ — Vectorscope &
  Waveform (very high demand; a viewer/scopes feature).
- `#163 <https://github.com/NatronGitHub/Natron/issues/163>`_ — Cryptomatte.
- `#319 <https://github.com/NatronGitHub/Natron/issues/319>`_ — 3D camera / 3D
  tracking.
- `#82 <https://github.com/NatronGitHub/Natron/issues/82>`_ — overlay node graph
  over the viewer.
- `#798 <https://github.com/NatronGitHub/Natron/issues/798>`_ — UI/UX + icons
  overhaul.
- `#283 <https://github.com/NatronGitHub/Natron/issues/283>`_ — Bodymovin/Lottie
  export (highest reaction count).

P3 — Good first issues and polish
---------------------------------

Well-scoped, low-risk tasks ideal for new contributors:
`#818 <https://github.com/NatronGitHub/Natron/issues/818>`_ (default Shuffle
input),
`#242 <https://github.com/NatronGitHub/Natron/issues/242>`_ (dot size),
`#502 <https://github.com/NatronGitHub/Natron/issues/502>`_ (retime node in Dope
Sheet display),
`#480 <https://github.com/NatronGitHub/Natron/issues/480>`_ (load user
parameters immediately),
`#826 <https://github.com/NatronGitHub/Natron/issues/826>`_ (embed timecode in
MOV),
`#601 <https://github.com/NatronGitHub/Natron/issues/601>`_ and
`#672 <https://github.com/NatronGitHub/Natron/issues/672>`_ (build-system, also
listed under P1).

Investigation starting points for the P0 bugs
---------------------------------------------

These are code-level leads for the highest-priority issues, correlated with the
source during the writing of this guide. They are hypotheses to accelerate a
maintainer, not confirmed root causes.

**OpenGL "won't open" / startup crashes — #516, #845, #1008.**
These reporters run AMD GPUs or particular Linux GL stacks, and the failure is at
launch, before a project loads. ``Engine/OSGLContext.cpp`` throws on
context-creation failure — a ``std::logic_error`` when no matching framebuffer
config is found (``chooseFBConfig``), and a ``std::runtime_error`` when the GL
version is insufficient (``checkOpenGLVersion``). If either exception is not
caught and turned into a graceful fallback, startup aborts.

Suggested approach: reproduce with a forced software/GL-fallback path, then catch
context-creation failures and fall back to software rendering with a clear
user-facing message. The README already documents ``LIBGL_ALWAYS_SOFTWARE=1`` on
Linux — wiring an automatic fallback would fix a whole class of "won't launch"
reports. Also review the per-platform backends (``OSGLContext_x11`` /
``OSGLContext_win``) and ``GPUContextPool`` initialization order relative to the
main window.

**Silent render stall with CPU→0 — #248.** The reporter sees rendering stop after
50–70 frames with CPU near zero (a wait, not a busy loop) — the signature of a
**deadlock or a lost wake-up**. Three loci are worth checking together:

- the scheduler threads — ``OutputSchedulerThread`` / ``GenericSchedulerThread``
  (the render-thread task queue and its condition variables) and
  ``AbortableRenderInfo``;
- cache-entry locking — ``CacheEntry`` / ``ImageLocker`` (a thread waiting on an
  entry that never becomes ready);
- the "being rendered elsewhere" wait in ``renderRoI``
  (``waitForImageBeingRenderedElsewhere``, see the worked trace in
  :ref:`maint-rendering`), where a thread can block on another thread's tile that
  never completes.

The memory/swap angle the reporter mentions suggests it triggers under cache
pressure, which overlaps with #192. Reproduce headless (``NatronRenderer``) under
a thread sanitizer or capture a stuck-thread backtrace.

**Cache ignores size limits, crashes on full disk — #192.** ``Engine/Cache.h``
tracks ``_maximumInMemorySize`` / ``_maximumCacheSize`` and computes an
``occupationPercentage`` to drive eviction, but the reporter shows the disk
cache growing past its configured limit until the disk fills. Audit the
disk-portion size accounting and eviction (the ``maximumDiskCacheSize`` branch)
and add a free-disk-space guard (``MemoryInfo``) that stops caching gracefully
instead of crashing. This likely interacts with #248.

**Teardown / repeated-open crashes — #795, #1029, #1057.** These are lifecycle
bugs (closing a project, or opening/closing repeatedly, or dismissing the Writer
file picker). Suspect destruction-order issues in the ``shared_ptr`` graph
(a ``weak_ptr`` that should be a strong ref during teardown, or vice-versa) and
GUI objects outliving the engine objects they reference through the ``…I``
interfaces. A debug build with ASan (``CONFIG+=addresssanitizer``) is the fastest
way in.

Maintenance recommendation
--------------------------

1. **Triage the untriaged.** ~half of open issues lack priority/difficulty
   labels; label them as reviewed so this analysis stays meaningful.
2. **Re-validate the 2018 import.** Bulk-verify the 124 issues imported in 2018
   against the current release and close the stale ones.
3. **Close the loop on ``status:fix-commited`` / ``status:in-progress``.** Verify
   and close (e.g. ``#195``) or unblock stalled work.
4. **Feed the crash reporter.** Prioritizing usable crash reports (``#557``)
   turns the long tail of "random crash" issues into fixable, stack-trace-backed
   bugs.
