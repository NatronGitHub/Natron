.. _maint-design:

Design Techniques and Idioms
============================

Natron uses a consistent set of C++ idioms throughout. Learning them once makes
the whole code base readable and tells you how to write code that fits in.

The ``Natron`` namespace and its macros
---------------------------------------

All Natron code lives in the ``Natron`` namespace, entered and exited through
macros defined in ``Global/Macros.h``::

    NATRON_NAMESPACE_ENTER
    // ... declarations ...
    NATRON_NAMESPACE_EXIT

Python-exposed classes live in a nested namespace opened with
``NATRON_PYTHON_NAMESPACE_ENTER``. Source files that need the namespace in
scope use ``NATRON_NAMESPACE_USING``. Using the macros (rather than writing
``namespace Natron { … }`` directly) lets the whole project's namespace be
renamed or instrumented from one place, and keeps the Shiboken binding
generator in sync.

``Global/Macros.h`` also holds the version numbers (``NATRON_VERSION_MAJOR`` /
``…_MINOR``, currently 2.6) and a family of **compiler-diagnostic macros** such
as ``GCC_DIAG_SUGGEST_OVERRIDE_OFF`` / ``…_ON`` and the CLANG/GCC ``push``/
``pop`` helpers. You will see these wrapped around ``Q_OBJECT`` and around
third-party includes to locally suppress warnings that Natron's strict flags
(``-Wall -Wextra`` and more) would otherwise raise on code Natron does not
control. When you add code that trips a spurious warning from Qt's generated
*moc* output (the Meta-Object Compiler that processes ``Q_OBJECT`` classes) or
from a bundled library, reach for these macros rather than weakening the global
warning flags.

Include Python.h first
----------------------

Every translation unit (directly or through an early header such as
``EngineFwd.h``) begins with::

    // ***** BEGIN PYTHON BLOCK *****
    #include <Python.h>
    // ***** END PYTHON BLOCK *****

This is mandated by CPython: ``Python.h`` must precede the standard headers
because it defines pre-processor symbols that affect them. Preserve this block
at the very top of files; moving it below other includes causes obscure build
failures, especially on Windows.

Smart pointers and the ``Fwd`` catalogs
---------------------------------------

Natron manages object lifetime almost entirely with ``std::shared_ptr`` and
``std::weak_ptr``. For every class ``Foo`` there is a typedef ``FooPtr``
(``shared_ptr<Foo>``) and usually ``FooWPtr`` (``weak_ptr<Foo>``), plus
``FooConstPtr`` where needed. These typedefs, together with the forward
declarations of every class, are centralized in two "catalog" headers:

- ``Engine/EngineFwd.h``
- ``Gui/GuiFwd.h``

Include the ``Fwd`` header (cheap) instead of a class's full header whenever you
only need to name the type — this is a major reason the project compiles at all
given its size. When you add a new class, add its forward declaration and
``…Ptr`` typedef to the appropriate catalog.

Two consequences to keep in mind:

- Classes that need to hand out a ``shared_ptr`` to themselves derive from
  ``std::enable_shared_from_this`` (``AppInstance``, ``Project``, and many
  others). Do not create a second ``shared_ptr`` from a raw ``this``.
- Parent→child links are ``shared_ptr``; child→parent (back-)references are
  ``weak_ptr`` to avoid reference cycles that would leak the whole graph. Follow
  this convention when adding relationships.

The PIMPL idiom
---------------

Most non-trivial classes hide their data and helpers behind a private
implementation object — the "pointer to implementation" (PIMPL) idiom. The
public header ``Foo.h`` declares ``class Foo`` with a single opaque member
(commonly ``boost::scoped_ptr<FooPrivate> _imp``, or ``std::unique_ptr`` in
newer code), and ``FooPrivate.h`` /
``FooPrivate.cpp`` hold the real data and logic. Examples are everywhere:
``NodePrivate``, ``GuiPrivate``, ``EffectInstancePrivate``,
``ViewerInstancePrivate``, ``RotoContextPrivate``, ``TrackerContextPrivate``,
``DockablePanelPrivate``.

PIMPL keeps public headers small and stable (changing a private member does not
force a recompile of every includer) and keeps heavy or platform-specific
includes out of the public interface. When you add state to a class, put it in
the ``…Private`` object, not the public header.

Engine/Gui decoupling: abstract "I" interfaces
----------------------------------------------

This is the most important structural pattern in Natron. The Engine sometimes
needs to *drive* the GUI — tell a node's widget to move, ask the viewer to
redraw, push an undo command — but it must not depend on ``Gui``. The solution
is a set of **pure-virtual interface classes**, by convention named with a
trailing ``I``, that live in ``Engine`` and are implemented in ``Gui``:

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Interface (in ``Engine``)
     - Implemented by (in ``Gui``)
     - Role
   * - ``NodeGuiI``
     - ``NodeGui``
     - a node's node-graph widget
   * - ``KnobGuiI``
     - ``KnobGui``
     - a parameter's on-screen widget
   * - ``OpenGLViewerI``
     - ``ViewerGL``
     - the OpenGL viewport
   * - ``NodeGraphI``
     - ``NodeGraph``
     - the node-graph canvas
   * - ``DockablePanelI``
     - ``DockablePanel``
     - a settings panel
   * - ``OverlaySupport`` / ``HostOverlaySupport``
     - viewer widgets
     - overlay drawing surface

The Engine holds pointers of the *interface* type (``NodeGuiIPtr``,
``OpenGLViewerI*``, …). At run time the GUI constructs the concrete object and
registers it; in headless mode nobody registers one and the Engine simply skips
the call. When you need the Engine to affect the GUI, add a method to the
relevant ``…I`` interface and implement it in the ``Gui`` class — never
``#include`` a ``Gui`` header from ``Engine``.

Signals and slots, and ``KnobSignalSlotHandler``
------------------------------------------------

Natron uses Qt's signal/slot mechanism heavily for decoupled notification —
even in the Engine (dozens of Engine classes are ``QObject`` subclasses with
``Q_OBJECT``). Change notifications (a knob's value changed, a node was
connected, a render step finished) propagate as signals.

Some core classes deliberately are **not** ``QObject`` — for example ``KnobI``
and its concrete knobs, which must be lightweight and copyable and are created
in large numbers. To give those classes signals without making them
``QObject``, Natron uses a companion object: ``KnobSignalSlotHandler`` is a
``QObject`` owned by each knob that emits the knob's signals on its behalf. If
you need a non-``QObject`` core class to notify observers, follow this
"signal-handler companion" pattern rather than converting the class to a
``QObject``.

Serialization with Boost
------------------------

Project files (``.ntp``) and clipboard data are written with
`Boost.Serialization <https://www.boost.org/doc/libs/release/libs/serialization/>`_
using the XML archive (``boost::archive::xml_iarchive`` /
``xml_oarchive``). Each serializable class has a companion
``FooSerialization.h`` describing what to read/write, and versioning is handled
with ``BOOST_CLASS_VERSION`` so that old projects still load.

.. warning::

   Serialization is **backward-compatibility-critical**. Any change to a
   ``…Serialization`` struct can break existing users' project files. Always
   bump the class version and add a version-guarded branch that can still read
   the old layout; never silently change field meaning or order. This is the
   single easiest place to cause data-loss regressions.

Thread-local storage for rendering
----------------------------------

Rendering runs on many threads and needs per-thread, per-render context (which
frame/view/region is being computed, the abort flag, statistics). Natron carries
this in thread-local storage via the ``TLSHolder`` template and ``AppTLS``, with
``ParallelRenderArgs`` as the per-render payload. This avoids threading that
context through hundreds of function signatures. See :ref:`maint-rendering`.

The singleton and the factory
-----------------------------

There is exactly one ``AppManager``, reached through the ``appPTR`` macro
(``AppManager::instance()``). It is the service locator for process-wide
facilities. Knobs are not constructed directly either: they are created through
``appPTR->getKnobFactory().createKnob<KnobType>(holder, …)`` so the factory can
wire up the knob's holder, GUI hook and signal handler consistently.

Cross-platform and toolkit shims
--------------------------------

Portability code is concentrated rather than scattered: ``Global/QtCompat.h``
for Qt-version differences, ``Global/StrUtils`` and ``FStreamsSupport`` for
string/stream portability (including a MinGW ``fstream`` workaround), the
``OSGLContext_*`` files for per-platform OpenGL contexts, and the ``glad`` loader
in ``Global/GLIncludes.h`` to obtain GL entry points. When you hit a
platform difference, add it to the appropriate shim rather than sprinkling
``#ifdef`` blocks through business logic.
