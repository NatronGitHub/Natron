# Distributing the AI Assistant build

## Why you cannot just hand over the .exe

The panel is compiled **into** `Natron.exe` (it is C++ under `Gui/`), not loaded as
a plug-in. There is no extension mechanism that would let someone add it to an
existing Natron install.

Dropping this `Natron.exe` into an official Natron install also fails outright:
this build links `libpython3.14.dll` plus 35 MSYS2 DLLs, while the official
installer ships Python 3.10 and its own DLL set. The executable will not load.

That is the cost of the C++ route (PRD #2, Option C). It buys grouped undo and a
native in-application panel; it is paid for at distribution time.

## The installer (what to hand someone)

`dist\NatronAI-Setup.exe` — **83.5 MB**, a normal Windows installer. Double-click,
next-next-finish, and there is a *Natron AI* icon on the desktop and in the Start
menu, plus an entry in Add/Remove Programs.

Build it after running the packaging script:

```
"%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" tools\natron-ai-installer.iss
```

### The shortcuts point straight at Natron.exe

No `.bat` wrapper and no environment variables, so nothing flashes a console
window on launch. That works because Natron resolves everything it needs relative
to its own executable:

- `Engine/OfxHost.cpp:890-892` — `applicationDirPath()`, `cdUp()`, then
  `/Plugins/OFX/Natron`. The package puts binaries in `bin\`, so this lands
  exactly on the shipped plug-ins.
- `Engine/Settings.cpp:100` — `<bin>/../Resources/OpenColorIO-Configs`.

Verified rather than assumed: 162 plug-ins load from an installed copy with
`OFX_PLUGIN_PATH` and `OCIO` unset and not even `bin` on `PATH`.

### Verified behaviour

- Silent install to a chosen directory: 4922 files, exit code 0.
- Desktop and Start-menu shortcuts created, targets valid.
- Uninstall removes everything — the log ends `Removed all? Yes` with zero files
  left. Note that Inno's uninstaller relaunches itself from `%TEMP%`, so the
  process you started returns immediately while deletion continues; checking the
  directory straight afterwards makes a clean uninstall look like a failure.
- An elevated (all-users) install puts the icon in `C:\Users\Public\Desktop`,
  which Windows shows on every user's desktop.
  `PrivilegesRequiredOverridesAllowed=dialog` lets someone without admin rights
  install into their own profile instead.

## What `tools/package-natron-ai.sh` produces

Run from the MSYS2 MINGW64 shell:

```bash
./tools/package-natron-ai.sh
```

Output: `dist/NatronAI/` (~406 MB) and `dist/NatronAI-win64.zip` (~143 MB).

The recipient unzips it anywhere and runs `Natron-AI.bat`. Nothing is installed;
it coexists with an official Natron install without touching it.

Verified self-contained: `NatronRenderer.exe --version` from the package succeeds
with `PATH` reduced to the package's own `bin` plus the Windows system
directories.

### What works

**162 plug-ins load, and an end-to-end render succeeds.** Verified by building
ColorBars -> Grade -> Write inside the packaged build and rendering one frame:
a 169 KB PNG came out. That exercises plug-in loading, node creation,
connection, parameter writes, rendering and file output.

- **openfx-misc**: Grade, Merge, Transform, ChromaKeyer, Keyer, PIK, Reformat,
  ColorCorrect, Crop, Shuffle, Premult, ImageStatistics and ~130 more.
- **openfx-io**: ReadOIIO / WriteOIIO (EXR, PNG, JPEG, TIFF, DPX, TGA, HDR...),
  ReadPNG / WritePNG, and the OCIO nodes.
- **Natron built-ins**: Roto, RotoPaint, Tracker, Group, Precomp, Backdrop, Dot.

Not included: GMIC and Arena (`openfx-gmic`, `openfx-arena`), the CImg nodes, and
**video read/write** — see below.

### Getting there: the GCC 15 / GCC 16 TLS conflict

`openfx-io` initially would not load at all, failing with `WinError 127`. Worth
recording, because anyone rebuilding this will hit it.

Diagnosis required comparing each binary's import table against the export tables
of the DLLs on disk, walking the whole dependency closure. `ldd` is useless here:
it refuses a `.ofx` (a DLL with a non-standard extension), and it only reports
missing *files*, whereas `WinError 127` is a missing *symbol*.

- `IO.ofx` and all of its direct imports resolved cleanly. The failure was three
  levels down.
- `libOpenImageIO-2.5.dll`, from Natron's pinned pacman repo and built with GCC
  15, imported `__emutls_v._ZSt11__once_call` and `__emutls_v._ZSt15__once_callable`
  — the *emulated-TLS* variables for `std::call_once`.
- GCC 16 moved MinGW to native TLS. Its libstdc++ exports
  `_ZSt15__get_once_callv` / `_ZSt19__get_once_callablev` **instead** and drops
  the emutls pair.
- Chain: `libOpenImageIO-2.5.dll` -> `libheif.dll` -> `libopenjph-0.31.dll`. The
  latter two come from current MSYS2 and are GCC 16 builds, so they need the
  native-TLS symbols.

So the two halves of the graph demanded mutually exclusive libstdc++ versions.
Downgrading to GCC 15 fixed OpenImageIO and broke libheif/libopenjph; every
`libheif` in the MSYS2 archive back to 1.22.0 is already a GCC 16 build, so no
version pair satisfied both.

**The fix was to rebuild OpenImageIO with GCC 16**, from Natron's own PKGBUILD:

```bash
cd tools/MINGW-packages/mingw-w64-openimageio
makepkg-mingw -sCLf --noconfirm --nocheck
pacman -U --overwrite '*' mingw-w64-x86_64-natron_openimageio-*.pkg.tar.zst
```

One patch is needed first. OIIO 2.5.13's `src/cmake/compiler.cmake:191` writes
`if (${CMAKE_SYSTEM_NAME} STREQUAL ...)`; with CMake 4 and an empty
`CMAKE_SYSTEM_PROCESSOR` that expands to a malformed `if` and configuration
aborts. Drop the `${}` so the variables are passed by name.

After that, rebuild `openfx-io` and everything resolves.

### Still missing: video I/O

`avcodec-58` from `natron_ffmpeg-gpl2` requires `libbluray-2.dll` and
`libx265-215.dll`; current MSYS2 ships `libbluray-3.dll` and `libx265-217.dll`.
Those are major soname bumps, so renaming them would risk crashes during decode
rather than fix anything. The `IO` bundle is therefore built with the FFmpeg
objects removed (`ReadFFmpeg.o FFmpegFile.o WriteFFmpeg.o PixelFormat.o` dropped
from `IO/Makefile`). Stills work fully; video does not.

The same treatment as OpenImageIO would fix it: rebuild `natron_ffmpeg-gpl2` from
its PKGBUILD against current MSYS2 codecs.

Unrelated but found alongside: `openfx-io/OIIO/Makefile` omits `ofxsFileOpen.o`,
so that sub-target never links. Patched locally.

## The alternative worth considering

If the goal is to get this in front of someone who **already has Natron**, the
C++ fork is the expensive way to do it. A pure-Python MCP server, dropped into
their existing install, needs no rebuild and no redistribution at all — Natron
bundles `json`, `socket`, `socketserver` and `threading`, and keeps `lib-dynload`
(`tools/jenkins/zip-python-mingw.sh:64-69, 85-92`), so it runs on the stock
binary.

It loses grouped undo (`QUndoStack::beginMacro` is not exposed to Python) and the
in-application panel, but it keeps node positions, because it runs in the GUI
process rather than headless — the limitation described for the headless route in
PRD #1 does not apply there.
