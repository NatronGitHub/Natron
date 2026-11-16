.. _maint-qt6:

Qt 6 Migration Plan
===================

Natron currently builds against **Qt 5** (it was migrated from Qt 4 earlier).
Adding **Qt 6** support — while keeping Qt 5 working — is a tracked goal
(issues `#1011 <https://github.com/NatronGitHub/Natron/issues/1011>`_ "QT6
Support?", and the Qt5 tracker `#827 <https://github.com/NatronGitHub/Natron/issues/827>`_).
This chapter is the concrete migration plan, based on an audit of the current
source tree.

.. note::

   Migration status and the occurrence counts below reflect an audit of the
   ``RB-2.6`` branch in mid-2026. They will change as the port progresses and as
   code is edited — treat the specific numbers as indicative and re-audit (a
   quick ``grep``) before relying on them. Move items to "done" as they land.

Groundwork already in place
---------------------------

The port is further along than it looks; the two hardest pieces are already
done:

- **The OpenGL viewer already uses ``QOpenGLWidget``.** ``ViewerGL``,
  ``Histogram``, ``CurveWidget``, ``DopeSheetView``, ``TimeLineGui`` and
  ``CustomParamInteract`` all derive from ``QOpenGLWidget``, not the Qt6-removed
  ``QGLWidget``. This is normally the single biggest Qt6 blocker for a GL-heavy
  app, and it is already handled.
- **The CMake build already has a Qt6 switch.** ``CMakeLists.txt`` exposes
  ``option(NATRON_QT6 "use Qt6" OFF)``. With it ``ON``, the build requires
  Qt 6.3 + ``OpenGLWidgets``, Shiboken6 and PySide6; with it ``OFF`` it uses
  Qt 5.15 + Shiboken2 + PySide2. So a dual-toolkit build is already modeled.
- **Binding scaffolding for PySide6 exists** (``PySide6_*_Python.h`` alongside
  ``PySide2_*_Python.h``).
- **``QtCompat.h`` already carries ``QT_VERSION_CHECK(6,0,0)`` shims** and
  ``QT_NO_CAST_FROM_ASCII`` is already defined project-wide, so implicit
  ``const char*`` → ``QString`` conversions (a common Qt6 trap) are already
  disallowed.

The strategy: build both from one source
----------------------------------------

Keep a single code base that compiles under Qt 5 and Qt 6, guarded where
necessary by ``#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)``. Prefer, in order:

1. **APIs that exist unchanged in both** (most code needs no change).
2. **A shim in ``Global/QtCompat.h``** when the two versions need different
   spellings — extend the existing pattern there rather than scattering version
   ``#if`` blocks through business logic.
3. **A localized ``#if QT_VERSION`` block** only when a shim is impractical.

Do the same for the two build systems: extend the qmake build with a
``CONFIG+=qt6`` (or ``QT_MAJOR_VERSION``-driven) path mirroring what CMake's
``NATRON_QT6`` already does, so both build systems can target either Qt.

Concrete work items (audited)
-----------------------------

The following are the actual Qt6-incompatible usages found in the tree today.
Counts are occurrences; see the named files.

``QRegExp`` → ``QRegularExpression`` (highest-effort item)
    ``QRegExp`` is removed from Qt 6 core (it survives only in the ``Qt5Compat``
    module, which is not a long-term option). **~33 uses across 16 files.**
    Engine: ``FileSystemModel.cpp`` (3), ``Markdown.cpp`` (3), ``Project.cpp``
    (2), ``OutputEffectInstance.cpp`` (2), ``NodeDocumentation.cpp`` (1),
    ``Node.cpp`` (1), ``CLArgs.cpp`` (1). Gui: ``ScriptTextEdit.cpp`` (6) +
    ``.h`` (1), ``RenderStatsDialog.cpp`` (3), ``FileTypeMainWindow_win.cpp``
    (3), ``NodeGraph45.cpp`` (2), ``NodeCreationDialog.cpp`` (2),
    ``PreferencesPanel.cpp`` (1), ``ViewerGL.cpp`` (1), ``SequenceFileDialog.h``
    (1). ``QRegularExpression`` has different semantics (anchoring, ``exactMatch``
    has no direct equivalent — use ``^…$`` or ``anchoredPattern()``), so each
    site needs review, not a blind substitution. ``QRegularExpression`` exists in
    Qt 5.15 too, so most conversions can be made **unconditionally** and will
    keep Qt 5 working.

``QDesktopWidget`` / ``QApplication::desktop()`` → ``QScreen`` (audit ~6 files)
    Removed in Qt 6. Referenced in ``GuiApplicationManager10.cpp`` (active — used
    for logical DPI), ``MessageBox.cpp``, ``Histogram.cpp``,
    ``NodeCreationDialog.cpp``, ``CurveWidget.cpp`` and ``ViewerGLPrivate.cpp`` —
    but audit each: several of these references are already commented out or
    partly migrated (``CurveWidget.cpp``, for example, already uses
    ``QGuiApplication::primaryScreen()`` in live code and only has a dead
    ``QDesktopWidget`` comment). For the remaining live uses, replace with
    ``QGuiApplication::screens()`` / ``QWidget::screen()`` /
    ``screen()->geometry()``. These APIs exist in Qt 5.15, so the change can be
    unconditional.

``QWidget::setMargin`` → ``setContentsMargins``
    A ``QLayout::setMargin(0)`` call in ``Gui/MessageBox.cpp`` (inside a
    non-Qt6 ``#else`` branch today). Deprecated; replace with
    ``setContentsMargins(0,0,0,0)``, which works on both Qt 5 and Qt 6.

Enum / ``QFlags`` scoping and the Python bindings (issue #854)
    Qt 6 (and PySide6) are stricter about scoped enums and ``QFlags``. Issue
    `#854 <https://github.com/NatronGitHub/Natron/issues/854>`_ ("Qt.Alignment
    Flags Unrecognized (As Int)") is exactly this: alignment flags passed as raw
    ``int`` no longer implicitly convert. Audit ``QtEnumConvert`` and any place
    that stores Qt flags as ``int`` (e.g. through ``QVariant``), and make sure
    the ``typesystem_*.xml`` files expose the flag types correctly to
    Shiboken6/PySide6. Regenerate the bindings with the Qt6 toolchain.

``QVariant`` API (1 site)
    One ``QVariant::Type`` use; Qt 6 deprecates ``QVariant::Type`` in favor of
    ``QMetaType``. Localized fix.

Shiboken6 / PySide6 binding regeneration
    Switch the binding generation to Shiboken6/PySide6 when ``NATRON_QT6`` is on
    (CMake already selects the toolchain). Regenerate the ``natronengine_*`` and
    ``natrongui_*`` wrappers and the ``devel/PythonReference`` docs. Validate
    against the tutorials and PyPlugs. This is where most *runtime* (as opposed
    to compile-time) surprises will appear.

Module/include changes
    ``QOpenGLWidget`` lives in the ``OpenGLWidgets`` module in Qt 6 (CMake
    already appends it). The qmake build must add ``QT += openglwidgets`` under
    a Qt6 condition. Audit ``QtOpenGL`` includes generally, since that module was
    reorganized in Qt 6.

Lower-risk, non-blocking cleanups
    ``foreach`` / ``Q_FOREACH`` (~46 uses) still compile under Qt 6 but are
    deprecated; migrate to range-based ``for`` opportunistically. ``QStyle``
    state flags (11) and ``Qt::SkipEmptyParts`` (already using the scoped form)
    are fine.

Verified replacement snippets
-----------------------------

The replacements below were **compiled against both Qt 5.15 and Qt 6** (Qt
5.15.19 and Qt 6.11.1 on macOS, ``clang++ -std=c++17 -fsyntax-only``, exit code
0 on both). They use only APIs present in both versions, so they can be applied
**unconditionally** — no ``#if QT_VERSION`` guard is needed, and the Qt 5 build
keeps working.

``QApplication::desktop()->screenGeometry()`` → primary ``QScreen``::

    // before (Qt 5, removed in Qt 6):
    QDesktopWidget* d = app->desktop();
    QRect g = d->screenGeometry();
    // after (Qt 5.15 and Qt 6):
    QRect g = QGuiApplication::primaryScreen()->geometry();

``desktop()->availableGeometry()`` for a widget's screen → ``QWidget::screen()``::

    // after: use the screen the widget is actually on (Qt 5.14+ and Qt 6)
    QScreen* s = widget->screen();
    QRect g = s ? s->availableGeometry()
                : QGuiApplication::primaryScreen()->availableGeometry();

``QRegExp`` exact match (e.g. the frame-range check in ``CLArgs.cpp``)::

    // before:  QRegExp re("[0-9\\-,]*"); bool ok = re.exactMatch(s);
    // after:   anchor the pattern (exactMatch has no direct equivalent)
    QRegularExpression re(QString::fromUtf8("\\A[0-9\\-,]*\\z"));
    bool ok = re.match(s).hasMatch();

``QRegExp`` capture (e.g. the backup-suffix regex in ``Project.cpp``)::

    // before:  QRegExp rx("\\.~(\\d+)~$"); rx.lastIndexIn(s); QString n = rx.cap(1);
    // after:
    QRegularExpression rx(QString::fromUtf8("\\.~(\\d+)~$"));
    QRegularExpressionMatch m = rx.match(s);
    QString n = m.hasMatch() ? m.captured(1) : QString();

``QRegExp`` wildcard (the ``QRegExp::WildcardUnix`` case in
``NodeCreationDialog.cpp`` — the one site that needs real thought)::

    // before:  QRegExp expr(pattern, Qt::CaseInsensitive, QRegExp::WildcardUnix);
    // after:
    QRegularExpression expr(
        QRegularExpression::wildcardToRegularExpression(pattern),
        QRegularExpression::CaseInsensitiveOption);
    bool matched = expr.match(text).hasMatch();

``QLayout::setMargin(0)`` (in ``MessageBox.cpp``) → ``setContentsMargins``::

    layout->setContentsMargins(0, 0, 0, 0);

.. note::

   ``QRegExp`` offered two matching modes: ``indexIn()`` found the pattern
   *anywhere* in the string, while ``exactMatch()`` required the *whole* string
   to match. ``QRegularExpression::match()`` is find-anywhere and has no
   ``exactMatch`` equivalent. So where the old code used ``indexIn``, the direct
   ``match()`` is correct; where it used ``exactMatch``, anchor the pattern with
   ``\A…\z`` (or ``QRegularExpression::anchoredPattern()``). Review each of the
   16 sites for which mode it relied on.

Suggested sequence
------------------

1. **Land the unconditional API modernizations first** (``QRegExp`` →
   ``QRegularExpression``, ``QDesktopWidget`` → ``QScreen``, ``setMargin``,
   ``QVariant::Type``). These improve the Qt 5 build too and shrink the eventual
   Qt6 diff. Verify the Qt 5 build and test suite stay green.
2. **Bring the qmake build to parity** with CMake's ``NATRON_QT6`` switch so
   both build systems can select Qt 6.
3. **Get a clean Qt 6 compile** with ``-DNATRON_QT6=ON``, fixing residual
   compile errors with ``QtCompat.h`` shims or narrow ``#if QT_VERSION`` guards.
4. **Regenerate and fix the PySide6 bindings**; resolve #854-class enum/flag
   issues at runtime.
5. **Validate the GUI end-to-end** on each OS: file dialog, node graph, viewer,
   properties panels, curve editor/dope sheet, roto/tracker overlays, and the
   Python console/panels. Mirror the checklist the Qt5 tracker (#827) used.
6. **Set up CI** to build both Qt 5 and Qt 6 so the dual-toolkit build does not
   regress (ties into the CI modernization work, issue
   `#601 <https://github.com/NatronGitHub/Natron/issues/601>`_).

Keeping Qt 5 working
--------------------

Qt 5.15 is the floor. Every change above either uses an API present in both
5.15 and 6.x (so no guard is needed) or is guarded by ``QT_VERSION``. Do not
drop the Qt 5 path until the Qt 6 build has shipped and been validated on all
three platforms — the same discipline that governed the Qt4→Qt5 migration.
