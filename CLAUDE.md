# CLAUDE.md — working notes for this repository

Natron is a node-based OpenFX-host video compositor (C++17 / Qt). This file is the
quick operating manual for working in this tree. The long-form reference is
`skills/natron-maintainer/SKILL.md` and `Documentation/source/maintainers/*.rst`.

## Layer stack — the one rule that must never break

```
libs/OpenFX + HostSupport   (OpenFX host)
        ↓
Engine                      (nodes, knobs, rendering, Python bindings)
        ↓
Gui                         (Qt UI)
        ↓
App (Natron)   Renderer (NatronRenderer)   PythonBin (natron-python)
```

**Dependencies point downward only. `Engine` must never include a header from
`Gui`.** `NatronRenderer` links `Engine` *without* `Gui`, so a single upward
include breaks the headless build. When Engine needs to call into the GUI, it goes
through an abstract interface (`NodeGuiI`, `KnobGuiI`, `OpenGLViewerI`,
`NodeGraphI`, `DockablePanelI` — the `FooI.h` files). Engine holds the interface;
Gui implements it.

## The two build systems — both must be updated

| | How sources are listed | Action needed when adding a file |
|---|---|---|
| **qmake** `Gui/Gui.pro` | explicit `SOURCES`/`HEADERS` lists (see `:196`, `:334`) | **edit manually**, alphabetical order |
| **CMake** `Gui/CMakeLists.txt` | `file(GLOB NatronGui_HEADERS *.h)` / `file(GLOB NatronGui_SOURCES *.cpp)` (`:21-22`) | **nothing** — picked up automatically |

Editing only one of the two is a listed Common Mistake. CMake is the preferred
build here:

```bash
cmake -S . -B build -DNATRON_QT6=OFF     # Qt5 + PySide2 (default)
cmake --build build -j
ctest --test-dir build
```

## Mandatory per-file idioms

**1. PYTHON BLOCK first.** `<Python.h>` must precede every other include in every
translation unit, with the standard comment banner:

```cpp
// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard
// headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****
```

**2. Namespace macros.** Everything lives between `NATRON_NAMESPACE_ENTER;` and
`NATRON_NAMESPACE_EXIT;` (defined in `Global/Macros.h`). Python-facing classes use
`NATRON_PYTHON_NAMESPACE_ENTER/EXIT`.

**3. `QT_NO_CAST_FROM_ASCII` is set.** A bare string literal will not compile into
a `QString`. Always `QString::fromUtf8("…")`, or `tr("…")` for user-visible text.

**4. PIMPL.** Public class `Foo` holds only an opaque pointer to `FooPrivate`.
In `Gui/` the convention is `std::unique_ptr<FooPrivate> _imp;` (see
`Gui/ScriptEditor.h:122`); older `Engine/` code uses `boost::scoped_ptr`.
The implementation struct lives in `FooPrivate.h` or at the top of `Foo.cpp`.
`FooPrivate` is a `struct` and is usually declared a `friend`.

**5. GPL header.** Every new file starts with the exact license block used by its
neighbours — copy it verbatim from an adjacent file in the same directory.

**6. Ownership.** `std::shared_ptr` down (parent→child), `std::weak_ptr` up
(child→parent). Register new types in the `Fwd` catalog for the layer
(`Engine/EngineFwd.h`, `Gui/GuiFwd.h`).

## File naming

| Suffix | Meaning |
|---|---|
| `FooFwd.h` | forward decls + `FooPtr`/`FooWPtr` typedefs; start here for unknown types |
| `FooI.h` | abstract interface — the Engine↔Gui seam |
| `FooPrivate.h/.cpp` | PIMPL body |
| `FooSerialization.h` | Boost.Serialization; bump `BOOST_CLASS_VERSION` on any change |
| `PyFoo.h/.cpp` | Shiboken-bound Python facade — **backward-compatibility-critical** |
| `OfxFoo.h/.cpp` | OpenFX host glue |
| `Gui20.cpp`, `ViewerTab30.cpp` | one class split across numbered files; numbers are grouping only |

## Runtime object tree

`appPTR` (`AppManager` singleton) → `AppInstance` (one per project) → `Project`
(a `NodeCollection` + `KnobHolder`) → `Node` → `EffectInstance`.

`Node` is the stable graph vertex; `EffectInstance` is its replaceable behaviour
(an `OfxEffectInstance`, or a built-in like `ViewerInstance`, `RotoPaint`,
`ReadNode`/`WriteNode`, `NodeGroup`, `Backdrop`, `Dot`).

Params are **knobs**: `KnobI` → `KnobHelper` → `KnobInt`/`KnobDouble`/…, owned by a
`KnobHolder`, created via `appPTR->getKnobFactory().createKnob<K>(...)`. Knobs are
**not** `QObject`s — each has a `KnobSignalSlotHandler` companion for signals.

## Threading

Node-graph mutation is **GUI-thread affine**. `Engine/Node.cpp` alone carries ~28
`assert(QThread::currentThread() == qApp->thread())` sites. These compile out in
release builds, so an off-thread call does **not** fail loudly — it races silently.
Anything arriving from a network or worker thread must hop to the GUI thread
(`Qt::BlockingQueuedConnection`) before touching nodes or knobs.

Note that in `NatronRenderer` there is **no Qt event loop at all**:
`AppManager::exec()` is called only from `Gui/GuiApplicationManager.cpp:957`.
Queued cross-thread signals to main-thread objects never fire headless.

## Corrections to older documentation

- **`natron-python` cannot `import NatronEngine`.** `SKILL.md` and
  `Documentation/source/maintainers/overview.rst:80-84` describe it as "a Python
  interpreter with Natron's modules". It is not. `PythonBin/python_main.cpp:66-84`
  is a bare `Py_Main` shim linking only `Python3::Python`
  (`PythonBin/CMakeLists.txt:27-32`); the `NatronEngine` module is registered with
  `PyImport_AppendInittab` inside `AppManager::initBuiltinPythonModules()`
  (`Engine/AppManager.cpp:3051-3057`), which that binary never calls, and
  `Engine/CMakeLists.txt:69` builds it `STATIC` — there is no importable module on
  disk. `natron-python` is useful only as a correct-version interpreter with a
  working `pip`. The Python API is reachable only from inside `Natron` or
  `NatronRenderer`.
- **Headless has no node positions.** `Node::setPosition` is a no-op without a GUI
  pointer and `getPosition` returns `0,0` (`Engine/Node.cpp:6303-6326`). Same for
  `setSize`/`setColor`.
- **Saving a project headless discards the GUI section permanently.**
  `Engine/Project.cpp:618-626` writes `Background_project=true` and skips
  `saveProjectGui()`; the loader reads that section only `if (!bgProject)`
  (`:331-333`). A GUI-authored `.ntp` re-saved headless loses node positions,
  colours and panel layout with no error.

## Style

```bash
astyle -p -H -f -j -z2 -c -k3 -U -A8 -n path/to/File.cpp
```
Enforced by `.git-hooks/pre-commit`.

## Common mistakes

- Including a `Gui` header from `Engine`.
- Updating `Gui/Gui.pro` but not `CMakeLists.txt`, or vice versa.
- Reading live node state during a render instead of the captured
  `ParallelRenderArgs`.
- Changing a `*Serialization` struct without a version bump.
- Making a core value type a `QObject` just to get signals.
- Commenting out code to make a build pass.
