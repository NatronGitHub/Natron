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

Qt Creator builds Natron with CMake, as in [INSTALL_WINDOWS.md](../../INSTALL_WINDOWS.md#build). Install the build dependencies listed there (or `${MINGW_PACKAGE_PREFIX}-natron-build-deps-qt5`), plus the debugger:

```
pacman -S --needed ${MINGW_PACKAGE_PREFIX}-gdb
```

Start Qt Creator and go to ``Tools`` => ``Options`` => ``Kits``. *It's recommended to click ``Apply`` after adding each item.*

* ``Debuggers``: add ``C:\msys64\ucrt64\bin\gdb.exe`` (``Name``: ``UCRT64 GDB``).
* ``Compilers``: add ``MinGW`` => ``C`` with ``C:\msys64\ucrt64\bin\gcc.exe`` (``UCRT64 C``), and ``MinGW`` => ``C++`` with ``C:\msys64\ucrt64\bin\g++.exe`` (``UCRT64 C++``).
* ``Qt Versions``: add ``C:\msys64\ucrt64\bin\qmake-qt5.exe`` (``UCRT64 Qt 5``). Qt Creator only uses it to locate Qt: MSYS2 renamed Qt 5's `qmake.exe` to `qmake-qt5.exe`, and the unsuffixed `qmake.exe` belongs to Qt 6.
* ``CMake``: add ``C:\msys64\ucrt64\bin\cmake.exe`` (``UCRT64 CMake``).
* ``Kits``: add a kit named ``Natron UCRT64`` with the C and C++ compilers, the debugger, the Qt version and the CMake tool above, and the ``Ninja`` CMake generator.

## Project

Clone the Natron source in the MSYS2 UCRT64 shell, and download the OpenColorIO configs (the same version as the CI, `OCIO_CONFIG_VERSION` in `.github/workflows/ci.yml`):

```
git clone https://github.com/NatronGitHub/Natron
cd Natron
git submodule update --init --recursive
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz
tar xvf Natron-v2.5.tar.gz
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```

Then open ``CMakeLists.txt`` from the Natron source folder in Qt Creator and select the ``Natron UCRT64`` kit. Under ``Projects`` => ``Natron UCRT64`` => ``Run`` => ``Environment``, add ``PYTHONHOME`` with the value ``C:\msys64\ucrt64``, and ``PYTHONLEGACYWINDOWSDLLLOADING`` with the value ``1`` (MSYS2's Python otherwise ignores ``PATH`` when it loads its DLLs, see #1147). Natron also needs ``C:\msys64\ucrt64\bin`` in ``PATH`` to find its DLLs.

You should now be able to build and run release and debug versions of Natron directly from Qt Creator. CMake options such as `-DNATRON_BUILD_TESTS=OFF` go in ``Projects`` => ``Build`` => ``CMake``.

# Setup a CI environment

The per-PR Windows CI (`.github/workflows/ci.yml`) builds Natron with CMake in UCRT64 from stock MSYS2 packages, and runs the unit tests with openfx-io's UCRT64 test build. To reproduce it locally, follow [Build](../../INSTALL_WINDOWS.md#build) in INSTALL_WINDOWS.md. It doesn't need Natron's package repository.

The release installer (`.github/workflows/build_installer.yml`, which runs `tools/jenkins/launchBuildMain.sh`) is built in UCRT64 too, with CMake (step 5 of #1133). The scripts in `tools/jenkins` take their paths, package names and architecture from the MSYS2 environment (`MINGW_PREFIX`, `MINGW_PACKAGE_PREFIX`, `MSYSTEM_CARCH`), never from `uname -m`, so that the same scripts can build for CLANGARM64 (#1494). Don't follow older instructions that set `MSYSTEM=MINGW64`.

The bundled DLLs are not listed by hand: `tools/jenkins/genDllVersions.sh` reads the import tables of the shipped binaries, Qt plug-ins and Python modules with `ntldd`, and bundles their dependency closure (the toolchain runtime included: libstdc++, libgcc_s and libgomp on UCRT64; libc++, libunwind and libomp on clang). Its report, `<installer>-dlls.txt` in the build archive, lists each DLL and where it comes from. A COMMERCIAL build fails if a GPL library would be bundled. `build_installer.yml` compares the file list and the DLLs of the new portable zip with the 2.6.0-alpha1 installer (the `compare-<installer>` artifact and the job summary).

A local installer build runs the same command as `build_installer.yml`, from `tools/jenkins` in a UCRT64 shell with `${MINGW_PACKAGE_PREFIX}-natron-build-deps-qt5` installed (see `install_natron_pacman_repo.sh` and "The package repository" below):

```
mkdir -p $HOME/natron-build
WORKSPACE=$HOME/natron-build BUILD_NAME=natron BUILD_NUMBER=1 BITS=64 NATRON_LICENSE=GPL \
  GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_URL_IS_NATRON=1 SNAPSHOT_BRANCH=RB-2.6 \
  QT_VERSION_MAJOR=5 DISABLE_BREAKPAD=1 NOUPDATE=1 MKJOBS=$(nproc) UNIT_TESTS=true \
  ./launchBuildMain.sh
```

The result goes to `$WORKSPACE/builds_archive`. See `launchBuildMain.sh` for the other options.


## The package repository

The Windows installer builds (`.github/workflows/build_installer.yml`) don't build the Natron packages (`mingw-w64-natron_*`, listed in `build_natron_package_repo.sh`) themselves. `.github/workflows/install_natron_pacman_repo.sh` downloads a prebuilt pacman repository, `natron_package_repo-<version>.zip`, from the `windows-mingw-package-repo` release, where `<version>` is the content of `windows_pacman_repo_version.txt` (`YYYYMMDD-N`).

The zip is looked up in this order:

1. the release of the repository the workflow runs from (`${GITHUB_SERVER_URL}/${GITHUB_REPOSITORY}`), so that a fork can publish its own rebuilt packages;
2. the upstream release, `https://github.com/NatronGitHub/Natron/releases/download/windows-mingw-package-repo/` (tried only once when the workflow runs on NatronGitHub/Natron).

A manual run of `build_installer.yml` can instead use the `natron_package_repo-<version>` artifact of a "Build pacman repo" run (input `pacman_repo_run_id`): the script then uses the repository that is already there. The repository of another MSYS2 environment than UCRT64 would be `natron_package_repo-<msystem>-<version>.zip` (e.g. `clangarm64`).

A download that fails, or that isn't a valid zip, is removed before trying the next location. If no zip is found, the script builds the whole repository locally with `build_natron_package_repo.sh`, which takes hours, and needs Inno Setup 6 (`ISCC.exe`, used by `mingw-w64-natron-setup`). `build_pacman_repo.yml` installs it; `build_installer.yml` doesn't, so on the installer workflow the fallback only works if Inno Setup 6 is already on the runner. The version is not pinned yet: `build_pacman_repo.yml` runs `choco install innosetup` without `--version`, so it gets whatever Chocolatey's latest is, and an Inno Setup 7 would break the build because the PKGBUILD hard-codes `Inno Setup 6/ISCC.exe`. It should be pinned to an exact 6.x release (Chocolatey needs a full version number, e.g. `choco install innosetup --version 6.4.3`; check which ones Chocolatey has).

`build_natron_package_repo.sh` downloads and verifies each package's sources (`makepkg --verifysource`) before building it, and retries that step once: the ffmpeg.org download times out from time to time. A build failure is not retried.

### Which packages Natron builds

The repository is built in MSYS2 UCRT64, and in CLANGARM64 for Windows on ARM64 (#1494, see "Windows on ARM64" below) (`build_pacman_repo.yml`, `build_natron_package_repo.sh`, which take the environment from the calling shell: `$MSYSTEM`, `$MINGW_PREFIX`, `$MINGW_PACKAGE_PREFIX`, and skip the packages whose `mingw_arch` doesn't list it). It contains only what MSYS2 doesn't provide in the form Natron and its plug-ins need:

| Package | Why Natron builds it |
|---|---|
| `freetype` 2.15.0-1.1 | MSYS2's FreeType 2.15.0-1 with the upstream fix [05f343a](https://gitlab.freedesktop.org/freetype/freetype/-/commit/05f343a) ("Fix macros", not in a release yet). FreeType 2.15.0 defines `ULABS( a )` without parentheses, so `FT_Set_Transform` resets a y flip combined with a rotation within about 1.8° of 45°, 135°, 225° or 315° to the identity, and rotated text comes out unrotated in openfx-arena's Text (#1287). It replaces MSYS2's package, see "Packages that replace MSYS2's" below. macOS has the same backport in `tools/MacPorts/print/freetype` (#1286). |
| `qt5-base` 5.15.19+kde+r96-2.1 (CLANGARM64 only) | MSYS2's qt5-base with the QMutex lost wake-up fix of #1453 (`1001-natron-qmutex-fix-lost-wakeup-arm64.patch`, the `-p1` version of the patch of [#1453](https://github.com/NatronGitHub/Natron/pull/1453), which is for MacPorts' qt5-qtbase). Qt 5's QMutex never uses futexes on Windows, and on a weakly ordered CPU (ARM64) a thread can sleep forever on an unlocked mutex (#1451). x86-64 is not affected in practice (every CAS is a full barrier there), so UCRT64 keeps MSYS2's. It replaces MSYS2's package, see "Packages that replace MSYS2's" below; delete it once MSYS2 ships the fix (a KDE `kde/5.15` backport, #1546). MSYS2's qt6-base (6.12) uses `WaitOnAddress` and is not affected. |
| `natron_ffmpeg-gpl2` 9.0.2 | FFmpeg for openfx-io, GPL-2.0-or-later (`--enable-gpl` without `--enable-version3`; MSYS2's FFmpeg is GPLv3), with the same version and configuration as Natron's macOS build (MacPorts `+gpl2 +natronmini`), in the private prefix `${MINGW_PREFIX}/ffmpeg-gpl2`. It uses MSYS2's `libx264` (ABI 165), so there is only one x264. |
| `natron_ffmpeg-lgpl` 9.0.2 | FFmpeg for openfx-io in COMMERCIAL builds, LGPL-2.1-or-later (#1302): MSYS2 has no LGPL FFmpeg. The configuration of `natron_ffmpeg-gpl2` without `--enable-gpl`, x264, x265, Xvid, vid.stab and libzvbi (the same as the macOS subport `ffmpeg-lgpl`), in `${MINGW_PREFIX}/ffmpeg-lgpl`. Its `check()` fails if `ffmpeg -L` or the libraries' licence strings are not LGPL 2.1. |
| `natron_libheif-lgpl` 1.23.7 | MSYS2's libheif links its encoders directly (`ENABLE_PLUGIN_LOADING=OFF`), including the GPL x264 and x265 (#1177). This is the same libheif without them, in `${MINGW_PREFIX}/libheif-lgpl`: HEIC is still decoded (libde265) and encoded (kvazaar), AVIF too; H.264 HEIF is only decoded. COMMERCIAL installers bundle its `libheif.dll` instead of MSYS2's (same name and ABI, so `natron_openimageio` loads it). Keep its version equal to MSYS2's libheif. |
| `natron_libtiff-lgpl` 4.7.2 | MSYS2's libtiff links JBIG-KIT (`libjbig`, GPL-2.0-or-later). This is the same libtiff without JBIG (LERC, libdeflate, lzma, zstd, WebP and JPEG are kept: their licences allow it), in `${MINGW_PREFIX}/libtiff-lgpl`. COMMERCIAL installers bundle its `libtiff-6.dll` instead of MSYS2's, and not `libjbig`; JBIG-compressed TIFF files can't be read there. Keep its version equal to MSYS2's libtiff. |
| `natron_libraw-gpl2`, `natron_libraw-lgpl` 0.22.3 | LibRaw with the GPL demosaic packs (GPL builds), and without them (COMMERCIAL builds). |
| `natron_openimageio` 3.1.17.0 | OIIO, the same version as Natron's macOS build, built against `natron_libraw-gpl2` and MSYS2's OpenColorIO, OpenEXR and Imath, without FFmpeg, for openfx-io. |
| `natron_seexpr-git` 2.11 | SeExpr 2.11 (the SeExpr 1 API), with Natron's build patches, for openfx-io. |
| `natron_imagemagick` 7.1.2-36 | ImageMagick 7, static, Q32 HDRI, no modules, with Magick++ and OpenMP (libgomp on UCRT64, libomp on CLANGARM64), with only the coders and delegates openfx-arena uses. MSYS2's imagemagick is a shared Q16 build with modules. |
| `osmesa` (Mesa 25.0.7) | Off-screen Mesa (llvmpipe) and GLU, built by [osmesa-install](https://github.com/devernay/osmesa-install) into `${MINGW_PREFIX}/osmesa`. MSYS2's Mesa no longer has OSMesa. It links MSYS2's static LLVM libraries and depends on the exact LLVM version it was built with: **rebuild it on every MSYS2 LLVM update** (until the repository also snapshots MSYS2's own packages, #1133). Only static libraries are shipped. |
| `natron-setup` | The Inno Setup installer, `${MINGW_PREFIX}/share/natron-setup/Setup.exe` (`/Setup.exe` before pkgrel 6): for x64 on UCRT64, for arm64 on CLANGARM64 (`NatronArch` in `Natron.iss`: `ArchitecturesAllowed=arm64`, and no Mesa component yet). |
| `natron-build-deps-common`, `-qt5`, `-qt6` | Meta-packages that install everything needed to build Natron, the plug-ins and the installer. |

OpenColorIO, OpenEXR, Imath, poppler (with the private headers that ReadPDF uses) and SoX now come from MSYS2. The Breakpad tools (`dump_syms`, `breakdown`) are gone: releases ship no crash reporter until #1183.

### Packages that replace MSYS2's

Natron's packages normally have their own names (`natron_*`, `osmesa`, `natron-*`), so that they can't clash with MSYS2's. A package that must replace an MSYS2 package for everything that links it (e.g. `freetype`, which harfbuzz, fontconfig, cairo, poppler and Qt depend on) keeps MSYS2's name instead, with MSYS2's PKGBUILD (`PKGBUILD.orig`), Natron's change (`PKGBUILD.patch`, regenerated with `diff -u PKGBUILD.orig PKGBUILD > PKGBUILD.patch`), and a `pkgrel` with a decimal part, such as `1.1` for MSYS2's `1`. It then installs over MSYS2's package with no file conflicts and no `provides`/`conflicts`. `check.sh` reports when MSYS2's PKGBUILD changes.

pacman takes a package from the *first* repository in `pacman.conf` that has it, without comparing versions, and `[natron]` comes after MSYS2's repositories. So neither `pacman -S` (or a dependency of `natron-build-deps-*`) nor `pacman -Syu` installs these packages from `[natron]`. They are listed in `NATRON_REPO_OVERRIDES` (`natron_repo_common.sh`), and `natron_repo_install_overrides` installs each of them explicitly, as `natron/<package>`, if and only if its version is newer than MSYS2's (`vercmp`). `build_natron_package_repo.sh` builds them first and installs them (without `--overwrite`, so that a file conflict fails the build), and `install_natron_pacman_repo.sh` installs them for the installer builds. Once installed, a later `pacman -Syu` keeps them ("local is newer than ucrt64") until MSYS2 has a higher version.

Because `1.1` sorts below `2`, an MSYS2 rebuild with the fix (`2.15.0-2`) or a new release (`2.15.1`) wins automatically: `natron_repo_install_overrides` then keeps MSYS2's package and emits a `::warning::` annotation, and `pacman -Syu` replaces Natron's. A package whose fix must not be lost (`NATRON_REPO_OVERRIDES_REQUIRED`: `qt5-base`, whose fix prevents the #1451 hang) makes `natron_repo_install_overrides` fail with an `::error::` instead, until MSYS2's package has been checked: delete Natron's, or rebase it on MSYS2's new PKGBUILD. When that happens, check that MSYS2's package has the fix, then delete the package: remove its directory, its entries in `build.sh`, `build_natron_package_repo.sh` and `NATRON_REPO_OVERRIDES`, and its row in the table above, and bump `windows_pacman_repo_version.txt`. Don't keep it around deprecated.

To install such a package by hand from a local build of the repository: `pacman -S natron/${MINGW_PACKAGE_PREFIX}-freetype`, or `pacman -U ${MINGW_PACKAGE_PREFIX}-freetype-*-any.pkg.tar.*` in its directory.

### When to rebuild the package repository

Rebuild it after changing anything under `tools/MINGW-packages`, and also when MSYS2 itself moves under the packages, even if no PKGBUILD changed. The packages in a zip are built against the MSYS2 of the day they were built, and break when MSYS2 changes ABI. For example, the `20250524-1` packages stopped loading in 2026:

- `natron_poppler` and `natron_openimageio` import `__emutls_v.*` symbols from libstdc++. They went away when MSYS2 moved GCC to native TLS, so `Arena.ofx` can't load.
- `natron_ffmpeg-gpl2` imports `libx265-215.dll` and `libbluray-2.dll`, which MSYS2 replaced with newer sonames (`libx265-217.dll`, `libbluray-3.dll`), so `IO.ofx` can't load.

Every package must then be rebuilt against the current toolchain (a full rebuild, not only the packages whose PKGBUILD changed). The symptom is an OFX plug-in that fails to load with "The specified module could not be found", e.g. in the "Verify plugin loading" step of the installer workflow. To see what a DLL imports, run `objdump -p some.dll | grep 'DLL Name'` in the MSYS2 shell.

### Rebuilding and publishing it

For CLANGARM64, run the workflow with `msystem` set to `CLANGARM64` (or `both`): it runs on GitHub's `windows-11-arm` runner, and its artifact and zip are `natron_package_repo-clangarm64-<version>` (`install_natron_pacman_repo.sh` looks for `natron_package_repo-<msystem>-<version>.zip` in environments other than UCRT64). `build_installer.yml` has the same `msystem` input.

1. On a branch, bump `windows_pacman_repo_version.txt` to a new version, e.g. today's date: `20261116-1` (increment the last number for a second build on the same day). Bump the `pkgrel` of any PKGBUILD whose output changes.
2. Run the "Build pacman repo" workflow on that branch (Actions → Build pacman repo → Run workflow; it also runs on `v*` tags). It runs `build_natron_package_repo.sh` on a clean runner and uploads the repository as the artifact `natron_package_repo-<version>`, named after the version read from `windows_pacman_repo_version.txt`. Check that the run passes: the job continues after a package failure so that its cache is saved, then fails in its last step.
3. Download the artifact. GitHub serves it as `natron_package_repo-<version>.zip`, with the repository files (`*.pkg.tar.*` and the repository database) at the top level, which is the layout that `install_natron_pacman_repo.sh` expects.
4. Upload it to the `windows-mingw-package-repo` release (maintainers only), e.g. `gh release upload windows-mingw-package-repo natron_package_repo-<version>.zip -R NatronGitHub/Natron`. A fork can upload it to a release with the same tag on the fork. Don't delete the zips of older versions: older branches and tags still use them.
5. Run "Build Installer" on the branch and check that it finds the zip and that the plug-ins load. Then merge the branch.

Until the new zip is published, every installer run on that branch falls back to building the whole repository locally.

## Windows on ARM64 (CLANGARM64)

Native ARM64 Natron (#1494) uses MSYS2's CLANGARM64 environment: clang, libc++, libunwind and LLVM's libomp (`llvm-openmp`), never gcc or MSVC. MSYS2 has no gcc, `gcc-libs` or `libgomp` there, so the PKGBUILDs depend on the virtual packages `cc`, `cc-libs` and `omp` (`omp` is provided by `libgomp` on UCRT64 and by `llvm-openmp` on CLANGARM64). Every MSYS2 package that Natron, its package repository and the plug-ins (openfx-misc, openfx-io, openfx-arena, openfx-gmic) use on UCRT64 exists in CLANGARM64, with the same versions (checked against the repository databases on 2026-11-20, see #1494). Only OSMesa is not an MSYS2 package on either environment (Natron's `osmesa`).

What is built differently there:
- `qt5-base`: Natron's, with the QMutex fix (see the table above).
- `natron_libraw-*`: `-lomp` is added to the link with clang, as MSYS2's libraw does.
- `natron_libtiff-lgpl`: `prepare()` runs `autoreconf -fiv` with clang, as MSYS2's libtiff does (libtool's clang support).
- `natron-setup`: Setup.exe is compiled with `/DNatronArch=arm64`.

Not verified on ARM64 hardware yet (#1494): OSMesa (llvmpipe) for Shadertoy, the OpenMP link of the static ImageMagick in openfx-arena, and a software OpenGL (Mesa `opengl32.dll`) component for the installer (MSYS2's mesa has an ARM64 `opengl32.dll`, but its `libgallium_wgl.dll` needs LLVM's DLL).

## Toolchain drift

MSYS2 is a rolling distribution, so the Windows build breaks from time to time with no change in Natron. These are the changes that broke it in 2026 (#1083), and how the build scripts deal with them. Check them first when the Windows build breaks again.

- **`qmake` was renamed `qmake-qt5`.** MSYS2 made Qt6 the default Qt and renamed Qt5's `qmake.exe` to `qmake-qt5.exe` (msys2/MINGW-packages 389f888c). `tools/jenkins/common.sh` (`find_qmake`) now looks for `qmake-qt${QT_VERSION_MAJOR}`, `qmake${QT_VERSION_MAJOR}`, then `qmake`, and sets and exports `QTDIR` and `QMAKE`. `launchBuildMain.sh` runs `$QMAKE -query QT_VERSION` early, prints e.g. `qmake-qt5 works (Qt 5.15.19)`, and stops with an error if it doesn't work or reports another Qt major version than `QT_VERSION_MAJOR` (e.g. Qt6's unsuffixed `qmake`, found when `qt5-base` is missing) (only a warning for plug-in CI builds, which don't need Qt). Before this, a wrong `QTDIR` only showed up much later, as `'QtGlobal' file not found` in shiboken.
- **Overriding qmake.** To use another qmake binary, set `NATRON_QMAKE` (e.g. `NATRON_QMAKE=/mingw64/bin/qmake-qt5.exe`). `QTDIR` is still the probed one. A `QMAKE` variable inherited from the environment is ignored, with a warning if it differs from the probe result, so that a value left over by a shell profile or another tool can't silently select the wrong Qt.
- **Two x264 ABIs** (until FFmpeg 9 replaced FFmpeg 4.4 and `natron_x264` was removed). MSYS2 renamed `x264-git` to `libx264`, which ships `libx264-165.dll` and is needed by MSYS2's `libheif`, while `natron_ffmpeg-gpl2` links `libx264-164.dll` from `natron_x264`. Both DLLs are needed, and both are bundled in GPL builds (`genDllVersions.sh` bundles them only when `NATRON_LICENSE=GPL`; COMMERCIAL builds bundle `natron_libheif-lgpl`, which doesn't link x264, see #1177): `genDllVersions.sh` uses `catAllDlls libx264-`, which emits every match, instead of `catDll`, which fails when more than one DLL matches. To avoid file conflicts with `libx264` (`x264.h`, `libx264.dll.a`, `x264.pc`), `natron_x264` installs into the private prefix `${MINGW_PREFIX}/natron_x264` and only puts `libx264-164.dll` in `${MINGW_PREFIX}/bin`. `natron_ffmpeg-gpl2` finds it through `PKG_CONFIG_PATH`. Note that `build_installer.yml` installs `natron-build-deps-qt5` with `pacman --overwrite "*"`, which hides this kind of conflict: only the "Build pacman repo" workflow, which installs packages without `--overwrite`, shows it. Since #1133 step 5, `genDllVersions.sh` takes the DLL names from the import tables (ntldd), so it needs no such special cases any more.
- **PySide major version.** `catDll libpyside` also matched `libpyside6` and `libpyside6qml` once Qt6 was installed next to Qt5 (`natron-build-deps-qt6`). PySide2 goes with Qt5 and PySide6 with Qt6, so the PySide major isn't `QT_VERSION_MAJOR`: `genDllVersions.sh` maps it explicitly (Qt5 → `libpyside2`/`libshiboken2`, Qt6 → `libpyside6`/`libshiboken6`), and matches `libpyside6.` and `libshiboken6.` (the main DLL names continue with a dot and the Python extension suffix) so that `libpyside6qml` isn't picked up. Since #1133 step 5, `genDllVersions.sh` takes the DLL names from the import tables (ntldd), so it needs no such special cases any more.
- **The bundled Python standard library.** `tools/jenkins/zip-python-mingw.sh` copies the packages listed in `PYDIRS`, plus the top-level `*.py` modules. Python 3.13 removed `lib2to3` (deprecated since 3.11) and `msilib` (one of the "dead batteries" of PEP 594): entries of `PYDIRS` that don't exist are now skipped, so names kept for older Pythons are harmless. Modules that become packages fall out of the `*.py` glob and silently disappear from the zip unless they are added to `PYDIRS`: `pathlib` and `sysconfig` (3.13), `string` (3.14), as well as the new `compression` package (3.14). The build doesn't fail, but the bundled interpreter can't `import string`. Check for this after every MSYS2 Python upgrade.
- **dump_syms with GCC 16** (the package was later removed, with Breakpad). The `mingw-w64-dump_syms` PKGBUILD built only the target that it packages (`make src/tools/windows/dump_syms_dwarf/dump_syms.exe`). `microdump_stackwalk`, which a bare `make` also builds, doesn't link with GCC 16 (undefined vtable for `google_breakpad::FastSourceLineResolver::Module`), because its target in breakpad's `Makefile.am` doesn't list `fast_source_line_resolver.o`.
- **PySide2 is gone from MINGW64.** On 2026-10-13 MSYS2 dropped `pyside2`, `shiboken2` and `pyside6` from the MINGW64 environment (msys2/MINGW-packages#32417). `natron-build-deps-qt5` depends on them, so it can no longer be installed on MINGW64, and the Windows installer and package-repository workflows (`build_installer.yml`, `build_pacman_repo.yml`) must move to UCRT64 (#1133).
- **Checking out a commit.** `tools/jenkins/checkout-repository.sh` can now check out a full 40-character commit sha (e.g. `NATRON_GIT_COMMIT`, `OPENFX_*_GIT_COMMIT`): it clones the branch (or the default branch) and fetches the commit. Before this, the sha was passed to `git clone -b`, which only accepts a branch or a tag.
