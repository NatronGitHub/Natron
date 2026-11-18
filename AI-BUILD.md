# Building the AI Assistant fork

## Status on this machine

**The build succeeds.** `[587/587] Linking CXX executable App\Natron.exe`, ninja
exit code 0, zero errors. All three new translation units compiled on the first
attempt and produced no warnings of their own (the build emits ~1618 warnings
overall, none from `AI*`):

| Object | Size |
|---|---|
| `AIMcpServer.cpp.obj` | 3.0 MB |
| `AIChatPanel.cpp.obj` | 1.3 MB |
| `AIAgentBackend.cpp.obj` | 1.0 MB |

`nm -C App/Natron.exe` finds **133** `AIMcpServer`/`AIChatPanel`/`AIAgentBackend`
symbols, so the panel really is linked into the application.

### ctest fails, and it is not this branch's doing

`ctest` reports `1 - Tests (SEGFAULT)`. That failure is environmental and
pre-existing. The proof is symbol-level rather than an argument:
`nm -C Tests/Tests.exe` finds **0** `AI*` symbols against 133 in `Natron.exe` —
`Tests/CMakeLists.txt:36-42` links `NatronEngine`, `Qt::Core`, `Python3::Python`
and `openMVG` but **not** `Gui`, and every change on this branch is under `Gui/`
or is documentation. The test binary does not contain this branch's code.

The actual cause is missing fixtures that CI supplies and a local checkout does
not: the OFX plug-in bundles at `../Plugins` (`.github/workflows/ci.yml:87`) and
`Resources/etc/fonts`. The test says so itself before dying: *You must specify
the filename of a script or Natron project (.ntp)* and *Fontconfig configuration
file ... does not exist*.

## What to install (Windows)

Natron on Windows builds under **MSYS2 / mingw64**, not MSVC. This is not a
preference: `Global/PythonUtils.cpp:80-84` expects a POSIX-layout Python
(`lib/python3.10/lib-dynload`), and the official installer ships MinGW CPython
(`tools/jenkins/zip-python-mingw.sh:14-16`).

The authoritative dependency set is the CI workflow
(`.github/workflows/ci.yml:136-147`), not `INSTALL_WINDOWS.md` (which is stale —
it still references `C:\Python34`).

1. Install MSYS2 from <https://www.msys2.org/>.
2. Open the **MINGW64** shell (not MSYS, not UCRT64) and run:

```bash
pacman -Syu
pacman -S --needed git unzip \
    mingw-w64-x86_64-wget \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-cmake
```

3. Add Natron's own pacman repository, which supplies the whole prebuilt
   dependency stack (Qt5, Boost, OpenFX, PySide2/Shiboken2, OpenColorIO, cairo,
   expat, ...). Running this from the repo root does it:

```bash
./.github/workflows/install_natron_pacman_repo.sh "$PWD" "$PWD/natron_pacman_repo"
pacman -S --needed --overwrite "*" mingw-w64-x86_64-natron-build-deps-qt5
```

4. Fetch the OpenColorIO configs (needed at runtime, and by the tests):

```bash
# The tag is 2.5 (ci.yml sets OCIO_CONFIG_VERSION: 2.5). Natron-v20210801 returns 404.
cd ..            # sibling of the Natron checkout
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz
tar xzf Natron-v2.5.tar.gz
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```

5. Submodules, then build:

```bash
git submodule update --init --recursive
mkdir -p build && cd build
cmake .. -G Ninja -DNATRON_QT6=OFF
ninja
```

`Gui/CMakeLists.txt:21-22` globs `*.h`/`*.cpp`, so the six new files are picked up
automatically. They were also added to `Gui/Gui.pro` by hand (lines 78-80 and
233-235) so the qmake build stays in sync, as
`skills/natron-maintainer/SKILL.md` requires.

## New files in this branch

| File | Purpose |
|---|---|
| `Gui/AIMcpServer.h/.cpp` | MCP (JSON-RPC 2.0) server over loopback HTTP with a bearer token; 15 tools (graph, params, expressions, keyframes, `plugin_search`, `script_exec`); grouped-undo transactions |
| `Gui/AIAgentBackend.h/.cpp` | Abstract agent backend + `ClaudeCodeBackend` driving the `claude` CLI over stream-json |
| `Gui/AIChatPanel.h/.cpp` | The dockable "AI Assistant" panel |

Modified: `Gui/Gui.h`, `Gui/Gui05.cpp`, `Gui/Gui40.cpp`, `Gui/GuiFwd.h`,
`Gui/GuiPrivate.h`, `Gui/GuiPrivate.cpp`, `Gui/TabWidget.h`, `Gui/TabWidget.cpp`,
`Gui/NodeGraph.h`, `Gui/Gui.pro`.

`Gui/NodeGraph.h` is the only pre-existing header whose semantics changed: the
`getUndoStack()` override moved from `private` to `public`. Access is checked
against the static type, so no existing caller is affected; the reason is that
grouping agent mutations into one macro requires `beginMacro`/`endMacro` on that
exact stack from outside the panel.

## API details that the build confirmed

These were read out of the headers while writing the code and are now also
compiler-verified. Worth keeping because several contradict the Python-facing
API, which is what most Natron documentation describes:

- `KnobChoice::getEntries_mt_safe()` returns `std::vector<ChoiceOption>` (not
  strings) — `Engine/KnobTypes.h:517`, `Engine/ChoiceOption.h:37-47`.
- `Node::getNInputs()` is the C++ spelling; `getMaxInputCount` is Python-only —
  `Engine/Node.h:322`.
- `Project::getProjectPath()/getProjectFilename()` return `QString`, not
  `std::string` — `Engine/Project.h:127,133`.
- `Node::destroyNode(bool blockingDestroy, bool autoReconnect)` —
  `Engine/Node.h:748`.
- `Knob<T>::setValue(const T&, ViewSpec, int)` — `Engine/Knob.h:1871`.
- `QUuid::WithoutBraces` needs Qt >= 5.11 (Natron targets 5.15).
- `Q_SIGNALS` is `public` in Qt5, so `AIMcpServerPrivate` may emit on behalf of
  its public interface. If a future Qt makes signals protected again, add small
  emit-helper methods on `AIMcpServer`.

Built here against gcc 16.2.0, CMake 4.4.2, Ninja 1.13.2, Qt5 + PySide2/Shiboken2
from the Natron pacman repo. Note that CMake resolved **Python 3.14.7 (MinGW)**,
not the 3.10 the official installers ship -- a concrete reminder that the bundled
Python version is not something to build assumptions on.

## Smoke-testing the MCP server without an agent

`tools/ai-mcp-smoketest.py` speaks the protocol directly. With Natron running and
the AI Assistant panel opened once (which starts the server), read the port and
token from the panel and run:

```bash
python tools/ai-mcp-smoketest.py http://127.0.0.1:PORT/mcp TOKEN
```

It runs `initialize`, `tools/list`, then `natron_status` and `graph_list_nodes`,
and prints what came back. That exercises the transport, the auth check, the
GUI-thread hop and the tool dispatch without involving any model.
