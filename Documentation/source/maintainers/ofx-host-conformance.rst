.. _ofx-host-conformance:

OpenFX 1.5.1 host conformance
=============================

This chapter records how well Natron, as an OpenFX host, follows what the
OpenFX 1.5.1 specification asks of hosts. It is meant for maintainers who fix
OFX host bugs, review changes to the host glue, or prepare Natron to advertise a
newer OFX API version. Natron now advertises OFX API 1.5
(``kOfxPropAPIVersion``, `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__);
it still advertised 1.4 at the audit commit 7dea437a6, and the tables below
describe that state unless a row says otherwise.

.. toctree::
    :maxdepth: 1

    ofx-host-conformance-core
    ofx-host-conformance-params
    ofx-host-conformance-imageeffect
    ofx-host-conformance-interact-gpu-packaging

What was audited
----------------

- **Specification:** OpenFX 1.5.1, the
  `OFX_Release_1.5.1 <https://github.com/AcademySoftwareFoundation/openfx/tree/OFX_Release_1.5.1>`__
  tag of AcademySoftwareFoundation/openfx: the headers in ``include/`` and the
  Reference and Guide pages in ``Documentation/sources/``.
- **Code:** Natron RB-2.6 at commit 7dea437a6, and HostSupport (the
  ``libs/OpenFX`` submodule, from NatronGitHub/openfx) at commit 93c21ed7,
  plus Natron's own extensions in ``libs/OpenFX_extensions``. Pull request
  `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__ (DrawSuite),
  which was still open, was reviewed separately.
- **Date:** 2026-11-19.

The audit is split into four areas, one page each:

- :ref:`ofx-host-conformance-core`: ``ofxCore.h`` (loading sequence, instance
  actions, general properties), and the property, memory, multithread,
  message, progress, timeline and dialog suites.
- :ref:`ofx-host-conformance-params`: ``ofxParam.h`` and
  ``ofxParametricParam.h``, including undo, persistence and project load.
- :ref:`ofx-host-conformance-imageeffect`: ``ofxImageEffect.h``: contexts,
  descriptor properties, clip preferences, clips and images, actions, render
  thread safety and the image-effect suite.
- :ref:`ofx-host-conformance-interact-gpu-packaging`: ``ofxInteract.h``,
  ``ofxKeySyms.h``, ``ofxDrawSuite.h``, ``ofxGPURender.h``, ``ofxColour.h``,
  and plug-in packaging, binary loading and the plug-in cache.

Method
------

Every statement of the specification that is addressed to the host (MUST,
SHOULD and MAY, and the documented default values) is one row of a table, with
the spec reference, its level, what Natron and HostSupport actually do (with
code references), and a verdict: OK, MISSING, WRONG, PARTIAL, N/A or NEXT (seen
but out of scope). Obligations of plug-ins are skipped unless the host has to
check them.

The audit is read-only: it is based on reading the code, ``git grep`` for other
callers and overrides, and ``git log -S`` / ``git log -L`` for the history of a
behaviour. No plug-in was run.

Each problem was verified in **two passes**. The first pass wrote the row. The
second pass (the "second check" in the problem lists) tried to prove the first
one wrong: it looked for other code paths, overrides, callers and settings that
could change the behaviour, and searched the existing issues. Only the problems
that survived the second pass were kept. They were then sorted by severity:
crash or data loss, wrong results or behaviour visible to plug-ins, and
spec-only deviations (return codes, handle checks, defaults).

Every crash or wrong-result problem was filed as its own issue (`#1361 <https://github.com/NatronGitHub/Natron/issues/1361>`__ to
`#1384 <https://github.com/NatronGitHub/Natron/issues/1384>`__); the spec-only deviations are grouped in the checklist of
`#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. Two existing
issues, `#909 <https://github.com/NatronGitHub/Natron/issues/909>`__ and
`#1359 <https://github.com/NatronGitHub/Natron/issues/1359>`__, were re-verified
and commented.

Keeping these pages up to date
------------------------------

- **When an issue is fixed**, change the verdict of its rows on the area page
  (for example "OK (fixed by #NNNN)", with a link to the pull request), add the
  same note to its entry in the Problems list of that page, and remove the
  issue from the open problems list on this page. Update the counts in both
  places: the Summary table below, and the area page's own counts section
  ("Verdict counts", or "Counts" on the parameters page; the core page has
  none). For an item of #1385, also tick its box in the issue.
- **Line numbers** in the code references are those of the audited commits
  (Natron 7dea437a6, HostSupport 93c21ed7); they are not updated when the code
  moves. When you re-check a row, update its code references and say at which
  commit.
- **Deliberate deviations** are marked "deliberate, won't fix". If one of them
  is changed after all, update its row like a fixed issue.
- **New spec versions:** when Natron moves to a newer OFX API (#1107), add rows
  for the new host statements and re-check the rows whose spec text changed.
- HostSupport fixes go to NatronGitHub/openfx first, followed by a submodule
  update in Natron; the issue stays open until the submodule is updated.

Summary
-------

Verdict counts per area, over the table rows. A row with a mixed verdict is
counted by its first word. The image-effect page has one row without a verdict
(GetClipPreferences, whose details are in other rows); the
interacts page does not count the rows describing pull request #1349.

.. list-table::
   :header-rows: 1
   :widths: 34 9 9 9 9 9 9 9

   * - Area
     - Rows
     - OK
     - PARTIAL
     - WRONG
     - MISSING
     - N/A
     - NEXT
   * - :ref:`Core <ofx-host-conformance-core>`
     - 99
     - 73
     - 12
     - 8
     - 1
     - 5
     - 0
   * - :ref:`Parameters <ofx-host-conformance-params>`
     - 105
     - 76
     - 15
     - 8
     - 0
     - 4
     - 2
   * - :ref:`Image effects <ofx-host-conformance-imageeffect>`
     - 111
     - 74
     - 20
     - 10
     - 2
     - 4
     - 0
   * - :ref:`Interacts, GPU, colour, packaging <ofx-host-conformance-interact-gpu-packaging>`
     - 72
     - 42
     - 13
     - 5
     - 4
     - 8
     - 0
   * - **Total**
     - 387
     - 265
     - 60
     - 31
     - 7
     - 21
     - 2

Open problems
-------------

Crash or data loss
~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 12 64 24

   * - Issue
     - Problem
     - Rows
   * - `#1361 <https://github.com/NatronGitHub/Natron/issues/1361>`__
     - Multithread-suite mutexes are created as ``QRecursiveMutex`` but
       locked, unlocked and destroyed as ``QMutex`` (Qt6 deadlock or crash).
     - Core T7
   * - `#1362 <https://github.com/NatronGitHub/Natron/issues/1362>`__
     - Parameter interacts are destroyed after their effect instance (use after
       free through ``kOfxPropEffectInstance``).
     - Interacts I13
   * - `#1363 <https://github.com/NatronGitHub/Natron/issues/1363>`__
     - An Unsafe plug-in loses its per-plug-in render lock when multithreading
       is turned off for it in the Preferences.
     - Image effects 6.1
   * - `#1364 <https://github.com/NatronGitHub/Natron/issues/1364>`__
     - ``kOfxImageEffectPluginPropSingleInstance`` is never honoured.
     - Image effects 2.1
   * - `#1365 <https://github.com/NatronGitHub/Natron/issues/1365>`__
     - A failed ``imageMemoryAlloc`` returns ``kOfxStatOK`` with an empty
       handle, and opens a modal dialog from a render thread.
     - Image effects 7.10
   * - `#1366 <https://github.com/NatronGitHub/Natron/issues/1366>`__
     - HostSupport ``clipGetHandle`` uses ``reinterpret_cast``, so a descriptor
       handle is used as an instance (undefined behaviour).
     - Image effects 7.3
   * - `#1367 <https://github.com/NatronGitHub/Natron/issues/1367>`__
     - Host-descriptor properties and HostSupport's private host pointer are
       writable by plug-ins.
     - Core H1, C7
   * - `#1368 <https://github.com/NatronGitHub/Natron/issues/1368>`__
     - Autosave serializes OFX nodes before SyncPrivateData runs, and the sync
       is skipped while a dialog is shown.
     - Core A15

Wrong results or behaviour visible to plug-ins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 12 64 24

   * - Issue
     - Problem
     - Rows
   * - `#909 <https://github.com/NatronGitHub/Natron/issues/909>`__
     - Begin/EndInstanceChanged never bracket parameter InstanceChanged
       actions (re-verified).
     - Core A7, A11
   * - `#1359 <https://github.com/NatronGitHub/Natron/issues/1359>`__
     - ``kOfxStatGPURenderFailed`` and ``kOfxStatGPUOutOfMemory`` are treated as
       plain failures, so the CPU retry never runs (re-verified).
     - Interacts G6
   * - `#1369 <https://github.com/NatronGitHub/Natron/issues/1369>`__
     - ``paramCopy`` copies nothing for 10 of the 13 value types (regression
       from 441a6d3d0).
     - Parameters S15, R13
   * - `#1370 <https://github.com/NatronGitHub/Natron/issues/1370>`__
     - The custom-param interpolation callback gets the param handle instead of
       the param set, and its status is ignored.
     - Parameters H1, T15, P32
   * - `#1371 <https://github.com/NatronGitHub/Natron/issues/1371>`__
     - ``kOfxParamPropIsAnimating`` and ``kOfxParamPropIsAutoKeying`` are only
       updated with a GUI, and only from dimension 0.
     - Parameters P5, P31
   * - `#1372 <https://github.com/NatronGitHub/Natron/issues/1372>`__
     - Params with ``kOfxParamPropEvaluateOnChange`` = 0 are never undoable.
     - Parameters P3, P8
   * - `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__
     - Undo of plug-in edits: a block starting with a CanUndo = 0 write merges
       into the previous undo step; the ``paramEditBegin`` label is ignored.
     - Parameters S16, S18, S19
   * - `#1374 <https://github.com/NatronGitHub/Natron/issues/1374>`__
     - Question messages answer "yes" when the user said no or was never asked.
     - Core G4
   * - `#1375 <https://github.com/NatronGitHub/Natron/issues/1375>`__
     - ``SequentialRenderStatus`` is 1 while frames render in parallel, and
       SequentialRender is honoured only for writers.
     - Image effects 2.6, 5.15, 5.16
   * - `#1376 <https://github.com/NatronGitHub/Natron/issues/1376>`__
     - With multiple clip PARs supported, every input gets the first input's
       pixel aspect ratio.
     - Image effects 3.9, 4.5
   * - `#1377 <https://github.com/NatronGitHub/Natron/issues/1377>`__
     - ``clipGetImage`` reuses an earlier image without comparing the region,
       and returns the same handle.
     - Image effects 7.4, 7.5
   * - `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__
     - Viewer overlay inputs: ``PenViewportPosition`` has a top-left origin,
       ``kOfxPropKeyString`` is always empty, pen pressure is 1 with no button
       down.
     - Interacts I6, I7, I21
   * - `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__
     - Parameter interacts never get the keyboard focus (no GainFocus/LoseFocus
       or key actions); empty key string, pressure always 1.
     - Interacts I7, I21, I24
   * - `#1380 <https://github.com/NatronGitHub/Natron/issues/1380>`__
     - Plug-in bundles whose name starts with ``@`` are loaded instead of
       skipped.
     - Interacts K7
   * - `#1381 <https://github.com/NatronGitHub/Natron/issues/1381>`__
     - ``kOfxActionEndInstanceEdit`` is sent without BeginInstanceEdit, and not
       when the last editor closes.
     - Core A16, A17
   * - `#1382 <https://github.com/NatronGitHub/Natron/issues/1382>`__
     - ``kOfxPropIsInteractive`` is always 0, even while the GUI is editing the
       effect.
     - Core A18
   * - `#1383 <https://github.com/NatronGitHub/Natron/issues/1383>`__
     - Plug-in contexts are not matched against the host's supported contexts
       (DescribeInContext is called for one context only).
     - Core C17; image effects 1.2, 1.3
   * - `#1384 <https://github.com/NatronGitHub/Natron/issues/1384>`__
     - OpenGL render is advertised as supported even when OpenGL is unavailable
       or disabled, and a failed ContextAttached is retried on every render.
     - Interacts G1, G10

Spec-only deviations
~~~~~~~~~~~~~~~~~~~~

`#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ is a checklist
of every remaining WRONG, PARTIAL or MISSING item that has no issue of its own:
wrong return codes, handles that are not validated, wrong defaults, properties
that are accepted but ignored. A few of them change what plug-ins see; they are
marked "[plug-in visible]" in the issue and can be split into their own issues
when someone works on them. The area pages link every such row to #1385.

Related issues that were already open: `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__ (advertise OFX API 1.5), `#1105 <https://github.com/NatronGitHub/Natron/issues/1105>`__
(thumbnail renders), `#1106 <https://github.com/NatronGitHub/Natron/issues/1106>`__ (OFX 1.5 / 1.5.1 / next: ChoiceOrder (1.5),
CPURenderSupported (1.5.1), InterpType and Obsolete (next)), `#1137 <https://github.com/NatronGitHub/Natron/issues/1137>`__ (ChoiceOrder), `#1145 <https://github.com/NatronGitHub/Natron/issues/1145>`__ (CUDA rendering), `#6 <https://github.com/NatronGitHub/Natron/issues/6>`__ (parameter
interacts), `#7 <https://github.com/NatronGitHub/Natron/issues/7>`__ (fields and field rendering) and `#87 <https://github.com/NatronGitHub/Natron/issues/87>`__ (Transition context).

Deliberate deviations (won't fix)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These deviations are kept on purpose, for compatibility with existing
plug-ins or projects. They are also listed at the end of #1385.

- Read-only properties are writable with a one-time warning
  (``ePluginReadOnlyWarn``), for old third-party plug-ins. Core R2.
- The host ``kOfxPropName`` is a user setting, and is replaced per plug-in by
  the Nuke or Vegas name for some vendors. Core P5.
- Host-level (NULL or descriptor handle) Error, Fatal and Warning messages go
  only to the log, because of Sapphire. Core G5.
- ``paramGetNumKeys`` and ``paramGetKeyTime`` report two fake keys when
  dimension 0 has an expression, so that generators see the animation.
  Parameters S10, problem 12.
- GetClipPreferences is called with mandatory clips disconnected, so that the
  layer menus can be built. Image effects 3.2.
- The default output premultiplication is "premultiplied" rather than
  "opaque" when no input is premultiplied, so that generators are not opaque.
  Image effects 3.12.
- The default RoI of a non-tiled plug-in or clip is the input RoD, and plug-ins
  without multi-resolution support get their connection refused rather than
  padded inputs. Image effects 5.4, 5.21.
- GetTimeDomain is also called in the Generator (and Reader and Writer)
  contexts, and its default includes optional inputs, for GenArts plug-ins.
  Image effects 5.7.
- PenUp and KeyUp are sent to every overlay even after one of them trapped
  the event, so that every overlay sees the release. Interacts I20.
