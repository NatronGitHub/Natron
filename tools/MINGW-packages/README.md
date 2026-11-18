# Natron Windows SDK

The following are instructions for installing and using the Natron SDK on Windows.

# Setup MSYS2

Download the latest installer from [msys2.org](https://www.msys2.org/).

Natron builds in MSYS2's **UCRT64** environment, MSYS2's default. After installation start ``C:\msys64\ucrt64.exe``. (The older MINGW64 environment is legacy, and MSYS2 no longer provides some of Natron's dependencies there, such as PySide2, OpenColorIO and OpenImageIO.)

```
pacman -Syu
pacman -S git
git clone https://github.com/NatronGitHub/Natron
cd Natron/tools/MINGW-packages
```

## Required packages

Most dependencies come from MSYS2 itself. A few are Natron's own builds, because Natron needs a pinned version, Natron patches, or a licence variant that MSYS2 doesn't provide (see "The package repository" below). The `natron-build-deps-qt5` (or `-qt6`) meta-package from Natron's package repository installs everything.

To build Natron's packages and install them locally (this takes a few hours):

```
./build_natron_package_repo.sh ~/natron_repo
pacman -S ${MINGW_PACKAGE_PREFIX}-natron-build-deps-qt5
```

To rebuild a single package:

```
cd mingw-w64-PACKAGE
../makepkg.sh
pacman -U ${MINGW_PACKAGE_PREFIX}-*-any.pkg.tar.*
```

# Setup a development environment (Qt Creator)

Start Qt Creator and go to ``Tools`` => ``Options``.

## Debuggers

Go to the ``Kits`` menu and select ``Debuggers``, then add a new.

* ``Name`` : ``MinGW GDB``
* ``Path`` : ``C:\msys64\ucrt64\bin\gdb.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Compilers

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C``).

* ``Name`` : ``MinGW C``
* ``Path`` : ``C:\msys64\ucrt64\bin\gcc.exe``

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C++``).

* ``Name`` : ``MinGW C++``
* ``Path`` : ``C:\msys64\ucrt64\bin\g++.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Qt

Go to the ``Kits`` menu and select ``Qt Versions``, then add a new.

* ``Name`` : ``MinGW Qt 4.8.7``
* ``Path`` : ``C:\msys64\ucrt64\bin\qmake.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Kit

Go to the ``Kits`` menu and select ``Kits``, then add a new.

* ``Name`` : ``Natron SDK Qt4``
* ``Compiler`` => ``C`` : ``MinGW C``
* ``Compiler`` => ``C++`` : ``MinGW C++``
* ``Debugger`` : ``MinGW GDB``
* ``Qt version`` : ``MinGW Qt 4.8.7``

## Project

Clone the Natron source (using Git Bash or MSYS2):

```
git clone https://github.com/NatronGitHub/Natron
cd Natron
git submodule update -i --recursive
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz
tar xvf Natron-v2.5.tar.gz
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
touch config.pri
```

Then start Qt Creator and open ``Project.pro`` from the Natron source folder and select the custom kit (``Natron MSYS2``) added previous. When Qt Creator is finished loading the project go to the ``Projects`` tab => ``Natron MSYS2`` => ``Build`` => ``Build Environment`` and add a new environment variable ``PYTHONHOME`` with the value ``C:\msys64\ucrt64``.

You should now be able to build and run release and debug versions of Natron directly from Qt Creator.

# Setup a CI environment

The installer scripts in `tools/jenkins` still use MINGW64; moving them to UCRT64 is the next step of #1133. Until then, with a working MSYS2 installation launch ``C:\msys64\mingw64.exe`` and clone the build tools:

```
git clone https://github.com/NatronGitHub/Natron NatronCI
```

Now create a build script:

```
#!/bin/bash
# Build Natron Windows Snapshot
#

export WORKSPACE=$HOME/NatronCI-workspace
export UNIT_TESTS=true
export BUILD_NAME=snapshots
export BUILD_NUMBER=RB-2.5
export GIT_URL=https://github.com/NatronGitHub/Natron.git
export SNAPSHOT_BRANCH=RB-2.5
export GIT_BRANCH=RB-2.5
export NATRON_LICENSE=GPL
export NATRON_DEV_STATUS=SNAPSHOT
export NATRON_BUILD_NUMBER=1
export COMPILE_TYPE=relwithdebinfo
export BITS=64
export DISABLE_BREAKPAD=1
export DISABLE_PORTABLE_ARCHIVE=0
export REMOTE_URL=NO_URL
export REMOTE_USER=NO_USER
export REMOTE_PREFIX=NO_PREFIX
export MKJOBS=4
export LOCALAPPDATA="/c/Users/USERNAME/AppData/Local"
export MSYSTEM=MINGW64
export NOUPDATE=1
export GIT_URL_IS_NATRON=1

if [ ! -d "$WORKSPACE" ]; then
    mkdir -p "$WORKSPACE"
fi

./launchBuildMain.sh
```

and save to ``$HOME/NatronCI/tools/jenkins/build-snapshot.sh``. Remember to change ``LOCALAPPDATA`` to match your username, and ``MKJOBS`` to match your CPU. You can disable unit tests with ``UNIT_TESTS=false``.

Now run the script *(working internet connection needed)*:

```
cd $HOME/NatronCI/tools/jenkins
bash build-snapshot.sh
```

The result will be in ``$HOME/NatronCI-workspace``.

This was just an example, see ``launchBuildMain.sh`` for more information/options.


## The package repository

The Windows installer builds (`.github/workflows/build_installer.yml`) don't build the Natron packages (`mingw-w64-natron_*`, listed in `build_natron_package_repo.sh`) themselves. `.github/workflows/install_natron_pacman_repo.sh` downloads a prebuilt pacman repository, `natron_package_repo-<version>.zip`, from the `windows-mingw-package-repo` release, where `<version>` is the content of `windows_pacman_repo_version.txt` (`YYYYMMDD-N`).

The zip is looked up in this order:

1. the release of the repository the workflow runs from (`${GITHUB_SERVER_URL}/${GITHUB_REPOSITORY}`), so that a fork can publish its own rebuilt packages;
2. the upstream release, `https://github.com/NatronGitHub/Natron/releases/download/windows-mingw-package-repo/` (tried only once when the workflow runs on NatronGitHub/Natron).

A download that fails, or that isn't a valid zip, is removed before trying the next location. If no zip is found, the script builds the whole repository locally with `build_natron_package_repo.sh`, which takes hours, and needs Inno Setup 6 (`ISCC.exe`, used by `mingw-w64-natron-setup`). `build_pacman_repo.yml` installs it; `build_installer.yml` doesn't, so on the installer workflow the fallback only works if Inno Setup 6 is already on the runner. The version is not pinned yet: `build_pacman_repo.yml` runs `choco install innosetup` without `--version`, so it gets whatever Chocolatey's latest is, and an Inno Setup 7 would break the build because the PKGBUILD hard-codes `Inno Setup 6/ISCC.exe`. It should be pinned to an exact 6.x release (Chocolatey needs a full version number, e.g. `choco install innosetup --version 6.4.3`; check which ones Chocolatey has).

`build_natron_package_repo.sh` downloads and verifies each package's sources (`makepkg --verifysource`) before building it, and retries that step once: the ffmpeg.org download times out from time to time. A build failure is not retried.

### Which packages Natron builds

The repository is built in MSYS2 UCRT64 (`build_pacman_repo.yml`, `build_natron_package_repo.sh`, which take the environment from the calling shell: `$MSYSTEM`, `$MINGW_PREFIX`, `$MINGW_PACKAGE_PREFIX`). It contains only what MSYS2 doesn't provide in the form Natron and its plug-ins need:

| Package | Why Natron builds it |
|---|---|
| `freetype` 2.15.0-1.1 | MSYS2's FreeType 2.15.0-1 with the upstream fix [05f343a](https://gitlab.freedesktop.org/freetype/freetype/-/commit/05f343a) ("Fix macros", not in a release yet). FreeType 2.15.0 defines `ULABS( a )` without parentheses, so `FT_Set_Transform` resets a y flip combined with a rotation within about 1.8° of 45°, 135°, 225° or 315° to the identity, and rotated text comes out unrotated in openfx-arena's Text (#1287). It replaces MSYS2's package, see "Packages that replace MSYS2's" below. macOS has the same backport in `tools/MacPorts/print/freetype` (#1286). |
| `natron_ffmpeg-gpl2` 9.0.2 | FFmpeg for openfx-io, GPL-2.0-or-later (`--enable-gpl` without `--enable-version3`; MSYS2's FFmpeg is GPLv3), with the same version and configuration as Natron's macOS build (MacPorts `+gpl2 +natronmini`), in the private prefix `${MINGW_PREFIX}/ffmpeg-gpl2`. It uses MSYS2's `libx264` (ABI 165), so there is only one x264. |
| `natron_libraw-gpl2`, `natron_libraw-lgpl` 0.22.3 | LibRaw with the GPL demosaic packs (GPL builds), and without them (COMMERCIAL builds). |
| `natron_openimageio` 3.1.17.0 | OIIO, the same version as Natron's macOS build, built against `natron_libraw-gpl2` and MSYS2's OpenColorIO, OpenEXR and Imath, without FFmpeg, for openfx-io. |
| `natron_seexpr-git` 2.11 | SeExpr 2.11 (the SeExpr 1 API), with Natron's build patches, for openfx-io. |
| `natron_imagemagick` 7.1.2-36 | ImageMagick 7, static, Q32 HDRI, no modules, with Magick++ and OpenMP (libgomp), with only the coders and delegates openfx-arena uses. MSYS2's imagemagick is a shared Q16 build with modules. |
| `osmesa` (Mesa 25.0.7) | Off-screen Mesa (llvmpipe) and GLU, built by [osmesa-install](https://github.com/devernay/osmesa-install) into `${MINGW_PREFIX}/osmesa`. MSYS2's Mesa no longer has OSMesa. It links MSYS2's static LLVM libraries and depends on the exact LLVM version it was built with: **rebuild it on every MSYS2 LLVM update** (until the repository also snapshots MSYS2's own packages, #1133). Only static libraries are shipped. |
| `natron-setup` | The Inno Setup installer. |
| `natron-build-deps-common`, `-qt5`, `-qt6` | Meta-packages that install everything needed to build Natron, the plug-ins and the installer. |

OpenColorIO, OpenEXR, Imath, poppler (with the private headers that ReadPDF uses) and SoX now come from MSYS2.

### Packages that replace MSYS2's

Natron's packages normally have their own names (`natron_*`, `osmesa`, `natron-*`), so that they can't clash with MSYS2's. A package that must replace an MSYS2 package for everything that links it (e.g. `freetype`, which harfbuzz, fontconfig, cairo, poppler and Qt depend on) keeps MSYS2's name instead, with MSYS2's PKGBUILD (`PKGBUILD.orig`), Natron's change (`PKGBUILD.patch`, regenerated with `diff -u PKGBUILD.orig PKGBUILD > PKGBUILD.patch`), and a `pkgrel` with a decimal part, such as `1.1` for MSYS2's `1`. It then installs over MSYS2's package with no file conflicts and no `provides`/`conflicts`. `check.sh` reports when MSYS2's PKGBUILD changes.

pacman takes a package from the *first* repository in `pacman.conf` that has it, without comparing versions, and `[natron]` comes after MSYS2's repositories. So neither `pacman -S` (or a dependency of `natron-build-deps-*`) nor `pacman -Syu` installs these packages from `[natron]`. They are listed in `NATRON_REPO_OVERRIDES` (`natron_repo_common.sh`), and `natron_repo_install_overrides` installs each of them explicitly, as `natron/<package>`, if and only if its version is newer than MSYS2's (`vercmp`). `build_natron_package_repo.sh` builds them first and installs them (without `--overwrite`, so that a file conflict fails the build), and `install_natron_pacman_repo.sh` installs them for the installer builds. Once installed, a later `pacman -Syu` keeps them ("local is newer than ucrt64") until MSYS2 has a higher version.

Because `1.1` sorts below `2`, an MSYS2 rebuild with the fix (`2.15.0-2`) or a new release (`2.15.1`) wins automatically: `natron_repo_install_overrides` then keeps MSYS2's package and prints a warning, and `pacman -Syu` replaces Natron's. When that happens, check that MSYS2's package has the fix, then delete the package: remove its directory, its entries in `build.sh`, `build_natron_package_repo.sh` and `NATRON_REPO_OVERRIDES`, and its row in the table above, and bump `windows_pacman_repo_version.txt`. Don't keep it around deprecated.

To install such a package by hand from a local build of the repository: `pacman -S natron/${MINGW_PACKAGE_PREFIX}-freetype`, or `pacman -U ${MINGW_PACKAGE_PREFIX}-freetype-*-any.pkg.tar.*` in its directory. The Breakpad tools (`dump_syms`, `breakdown`) are gone: releases ship no crash reporter until #1183.

### When to rebuild the package repository

Rebuild it after changing anything under `tools/MINGW-packages`, and also when MSYS2 itself moves under the packages, even if no PKGBUILD changed. The packages in a zip are built against the MSYS2 of the day they were built, and break when MSYS2 changes ABI. For example, the `20250524-1` packages stopped loading in 2026:

- `natron_poppler` and `natron_openimageio` import `__emutls_v.*` symbols from libstdc++. They went away when MSYS2 moved GCC to native TLS, so `Arena.ofx` can't load.
- `natron_ffmpeg-gpl2` imports `libx265-215.dll` and `libbluray-2.dll`, which MSYS2 replaced with newer sonames (`libx265-217.dll`, `libbluray-3.dll`), so `IO.ofx` can't load.

Every package must then be rebuilt against the current toolchain (a full rebuild, not only the packages whose PKGBUILD changed). The symptom is an OFX plug-in that fails to load with "The specified module could not be found", e.g. in the "Verify plugin loading" step of the installer workflow. To see what a DLL imports, run `objdump -p some.dll | grep 'DLL Name'` in the MSYS2 shell.

### Rebuilding and publishing it

1. On a branch, bump `windows_pacman_repo_version.txt` to a new version, e.g. today's date: `20261116-1` (increment the last number for a second build on the same day). Bump the `pkgrel` of any PKGBUILD whose output changes.
2. Run the "Build pacman repo" workflow on that branch (Actions → Build pacman repo → Run workflow; it also runs on `v*` tags). It runs `build_natron_package_repo.sh` on a clean runner and uploads the repository as the artifact `natron_package_repo-<version>`, named after the version read from `windows_pacman_repo_version.txt`. Check that the run passes: the job continues after a package failure so that its cache is saved, then fails in its last step.
3. Download the artifact. GitHub serves it as `natron_package_repo-<version>.zip`, with the repository files (`*.pkg.tar.*` and the repository database) at the top level, which is the layout that `install_natron_pacman_repo.sh` expects.
4. Upload it to the `windows-mingw-package-repo` release (maintainers only), e.g. `gh release upload windows-mingw-package-repo natron_package_repo-<version>.zip -R NatronGitHub/Natron`. A fork can upload it to a release with the same tag on the fork. Don't delete the zips of older versions: older branches and tags still use them.
5. Run "Build Installer" on the branch and check that it finds the zip and that the plug-ins load. Then merge the branch.

Until the new zip is published, every installer run on that branch falls back to building the whole repository locally.

## Toolchain drift

MSYS2 is a rolling distribution, so the Windows build breaks from time to time with no change in Natron. These are the changes that broke it in 2026 (#1083), and how the build scripts deal with them. Check them first when the Windows build breaks again.

- **`qmake` was renamed `qmake-qt5`.** MSYS2 made Qt6 the default Qt and renamed Qt5's `qmake.exe` to `qmake-qt5.exe` (msys2/MINGW-packages 389f888c). `tools/jenkins/common.sh` (`find_qmake`) now looks for `qmake-qt${QT_VERSION_MAJOR}`, `qmake${QT_VERSION_MAJOR}`, then `qmake`, and sets and exports `QTDIR` and `QMAKE`. `launchBuildMain.sh` runs `$QMAKE -query QT_VERSION` early, prints e.g. `qmake-qt5 works (Qt 5.15.19)`, and stops with an error if it doesn't work or reports another Qt major version than `QT_VERSION_MAJOR` (e.g. Qt6's unsuffixed `qmake`, found when `qt5-base` is missing) (only a warning for plug-in CI builds, which don't need Qt). Before this, a wrong `QTDIR` only showed up much later, as `'QtGlobal' file not found` in shiboken.
- **Overriding qmake.** To use another qmake binary, set `NATRON_QMAKE` (e.g. `NATRON_QMAKE=/mingw64/bin/qmake-qt5.exe`). `QTDIR` is still the probed one. A `QMAKE` variable inherited from the environment is ignored, with a warning if it differs from the probe result, so that a value left over by a shell profile or another tool can't silently select the wrong Qt.
- **Two x264 ABIs** (until FFmpeg 9 replaced FFmpeg 4.4 and `natron_x264` was removed). MSYS2 renamed `x264-git` to `libx264`, which ships `libx264-165.dll` and is needed by MSYS2's `libheif`, while `natron_ffmpeg-gpl2` links `libx264-164.dll` from `natron_x264`. Both DLLs are needed, and both are bundled in GPL builds (`genDllVersions.sh` bundles them only when `NATRON_LICENSE=GPL`; COMMERCIAL builds lack `libx264-165.dll`, see #1177): `genDllVersions.sh` uses `catAllDlls libx264-`, which emits every match, instead of `catDll`, which fails when more than one DLL matches. To avoid file conflicts with `libx264` (`x264.h`, `libx264.dll.a`, `x264.pc`), `natron_x264` installs into the private prefix `${MINGW_PREFIX}/natron_x264` and only puts `libx264-164.dll` in `${MINGW_PREFIX}/bin`. `natron_ffmpeg-gpl2` finds it through `PKG_CONFIG_PATH`. Note that `build_installer.yml` installs `natron-build-deps-qt5` with `pacman --overwrite "*"`, which hides this kind of conflict: only the "Build pacman repo" workflow, which installs packages without `--overwrite`, shows it.
- **PySide major version.** `catDll libpyside` also matched `libpyside6` and `libpyside6qml` once Qt6 was installed next to Qt5 (`natron-build-deps-qt6`). PySide2 goes with Qt5 and PySide6 with Qt6, so the PySide major isn't `QT_VERSION_MAJOR`: `genDllVersions.sh` maps it explicitly (Qt5 → `libpyside2`/`libshiboken2`, Qt6 → `libpyside6`/`libshiboken6`), and matches `libpyside6.` and `libshiboken6.` (the main DLL names continue with a dot and the Python extension suffix) so that `libpyside6qml` isn't picked up.
- **The bundled Python standard library.** `tools/jenkins/zip-python-mingw.sh` copies the packages listed in `PYDIRS`, plus the top-level `*.py` modules. Python 3.13 removed `lib2to3` (deprecated since 3.11) and `msilib` (one of the "dead batteries" of PEP 594): entries of `PYDIRS` that don't exist are now skipped, so names kept for older Pythons are harmless. Modules that become packages fall out of the `*.py` glob and silently disappear from the zip unless they are added to `PYDIRS`: `pathlib` and `sysconfig` (3.13), `string` (3.14), as well as the new `compression` package (3.14). The build doesn't fail, but the bundled interpreter can't `import string`. Check for this after every MSYS2 Python upgrade.
- **dump_syms with GCC 16** (the package was later removed, with Breakpad). The `mingw-w64-dump_syms` PKGBUILD built only the target that it packages (`make src/tools/windows/dump_syms_dwarf/dump_syms.exe`). `microdump_stackwalk`, which a bare `make` also builds, doesn't link with GCC 16 (undefined vtable for `google_breakpad::FastSourceLineResolver::Module`), because its target in breakpad's `Makefile.am` doesn't list `fast_source_line_resolver.o`.
- **PySide2 is gone from MINGW64.** On 2026-10-13 MSYS2 dropped `pyside2`, `shiboken2` and `pyside6` from the MINGW64 environment (msys2/MINGW-packages#32417). `natron-build-deps-qt5` depends on them, so it can no longer be installed on MINGW64, and the Windows installer and package-repository workflows (`build_installer.yml`, `build_pacman_repo.yml`) must move to UCRT64 (#1133).
- **Checking out a commit.** `tools/jenkins/checkout-repository.sh` can now check out a full 40-character commit sha (e.g. `NATRON_GIT_COMMIT`, `OPENFX_*_GIT_COMMIT`): it clones the branch (or the default branch) and fetches the commit. Before this, the sha was passed to `git clone -b`, which only accepts a branch or a tag.
