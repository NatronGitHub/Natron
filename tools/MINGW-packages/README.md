# Natron Windows SDK

The following are instructions for installing and using the Natron SDK on Windows.

# Setup MSYS2

Download latest installer from [msys2.org](https://www.msys2.org/).

After installation start ``C:\msys64\mingw64.exe``.

```
pacman -Syu
pacman -S git
git clone https://github.com/NatronGitHub/Natron
cd Natron/tools/MINGW-packages
```

## Required packages

Several packages are required to use the SDK.

```
pacman -S `cat msys2.txt`
./get.sh
```

In some cases the `get.sh` command may provide incompatible packages, try building the failed package(s):

```
cd mingw-w64-PACKAGE
../makepkg.sh
pacman -U mingw-w64-x86_64-*-any.pkg.tar.*
```

or build everything:

```
./build.sh
```

# Setup a development environment (Qt Creator)

Start Qt Creator and go to ``Tools`` => ``Options``.

## Debuggers

Go to the ``Kits`` menu and select ``Debuggers``, then add a new.

* ``Name`` : ``MinGW GDB``
* ``Path`` : ``C:\msys64\mingw64\bin\gdb.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Compilers

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C``).

* ``Name`` : ``MinGW C``
* ``Path`` : ``C:\msys64\mingw64\bin\gcc.exe``

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C++``).

* ``Name`` : ``MinGW C++``
* ``Path`` : ``C:\msys64\mingw64\bin\g++.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Qt

Go to the ``Kits`` menu and select ``Qt Versions``, then add a new.

* ``Name`` : ``MinGW Qt 4.8.7``
* ``Path`` : ``C:\msys64\mingw64\bin\qmake.exe``

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

Then start Qt Creator and open ``Project.pro`` from the Natron source folder and select the custom kit (``Natron MSYS2``) added previous. When Qt Creator is finished loading the project go to the ``Projects`` tab => ``Natron MSYS2`` => ``Build`` => ``Build Environment`` and add a new environment variable ``PYTHONHOME`` with the value ``C:\msys64\mingw64``.

You should now be able to build and run release and debug versions of Natron directly from Qt Creator.

# Setup a CI environment

With a working MSYS2 installation launch ``C:\msys64\mingw64.exe`` and clone the build tools:

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

A download that fails, or that isn't a valid zip, is removed before trying the next location. If no zip is found, the script builds the whole repository locally with `build_natron_package_repo.sh`, which takes hours, and needs Inno Setup 6 (`ISCC.exe`, used by `mingw-w64-natron-setup`). `build_pacman_repo.yml` installs it; `build_installer.yml` doesn't, so on the installer workflow the fallback only works if Inno Setup 6 is already on the runner. When installing Inno Setup with Chocolatey, pin the major version (`choco install innosetup --version 6.x.y`), since the PKGBUILD hard-codes `Inno Setup 6/ISCC.exe`.

`build_natron_package_repo.sh` downloads and verifies each package's sources (`makepkg --verifysource`) before building it, and retries that step once: the ffmpeg.org download times out from time to time. A build failure is not retried.

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

- **`qmake` was renamed `qmake-qt5`.** MSYS2 made Qt6 the default Qt and renamed Qt5's `qmake.exe` to `qmake-qt5.exe` (msys2/MINGW-packages 389f888c). `tools/jenkins/common.sh` (`find_qmake`) now looks for `qmake-qt${QT_VERSION_MAJOR}`, `qmake${QT_VERSION_MAJOR}`, then `qmake`, and sets and exports `QTDIR` and `QMAKE`. `launchBuildMain.sh` runs `$QMAKE -query QT_VERSION` early, prints e.g. `qmake-qt5 works (5.15.19)`, and stops with an error if it doesn't work (only a warning for plug-in CI builds, which don't need Qt). Before this, a wrong `QTDIR` only showed up much later, as `'QtGlobal' file not found` in shiboken.
- **Overriding qmake.** To use another qmake binary, set `NATRON_QMAKE` (e.g. `NATRON_QMAKE=/mingw64/bin/qmake-qt5.exe`). `QTDIR` is still the probed one. A `QMAKE` variable inherited from the environment is ignored, with a warning if it differs from the probe result, so that a value left over by a shell profile or another tool can't silently select the wrong Qt.
- **Two x264 ABIs.** MSYS2 renamed `x264-git` to `libx264`, which ships `libx264-165.dll` and is needed by MSYS2's `libheif`, while `natron_ffmpeg-gpl2` links `libx264-164.dll` from `natron_x264`. Both DLLs are needed and bundled: `genDllVersions.sh` uses `catAllDlls libx264-`, which emits every match, instead of `catDll`, which fails when more than one DLL matches. To avoid file conflicts with `libx264` (`x264.h`, `libx264.dll.a`, `x264.pc`), `natron_x264` installs into the private prefix `${MINGW_PREFIX}/natron_x264` and only puts `libx264-164.dll` in `${MINGW_PREFIX}/bin`. `natron_ffmpeg-gpl2` finds it through `PKG_CONFIG_PATH`. Note that `build_installer.yml` installs `natron-build-deps-qt5` with `pacman --overwrite "*"`, which hides this kind of conflict: only the "Build pacman repo" workflow, which installs packages without `--overwrite`, shows it.
- **PySide major version.** `catDll libpyside` also matched `libpyside6` and `libpyside6qml` once Qt6 was installed next to Qt5 (`natron-build-deps-qt6`). PySide2 goes with Qt5 and PySide6 with Qt6, so the PySide major isn't `QT_VERSION_MAJOR`: `genDllVersions.sh` maps it explicitly (Qt5 → `libpyside2`/`libshiboken2`, Qt6 → `libpyside6`/`libshiboken6`).
- **The bundled Python standard library.** `tools/jenkins/zip-python-mingw.sh` copies the packages listed in `PYDIRS`, plus the top-level `*.py` modules. PEP 594 removed `lib2to3` and `msilib` in Python 3.13: entries of `PYDIRS` that don't exist are now skipped, so names kept for older Pythons are harmless. Modules that become packages fall out of the `*.py` glob and silently disappear from the zip unless they are added to `PYDIRS`: `pathlib` (3.13), `string` and `sysconfig` (3.14), as well as the new `compression` package (3.14). The build doesn't fail, but the bundled interpreter can't `import string`. Check for this after every MSYS2 Python upgrade.
- **dump_syms with GCC 16.** The `mingw-w64-dump_syms` PKGBUILD now builds only the target that it packages (`make src/tools/windows/dump_syms_dwarf/dump_syms.exe`). `microdump_stackwalk`, which a bare `make` also builds, doesn't link with GCC 16 (undefined vtable for `google_breakpad::FastSourceLineResolver::Module`), because its target in breakpad's `Makefile.am` doesn't list `fast_source_line_resolver.o`.
- **Checking out a commit.** `tools/jenkins/checkout-repository.sh` can now check out a full 40-character commit sha (e.g. `NATRON_GIT_COMMIT`, `OPENFX_*_GIT_COMMIT`): it clones the branch (or the default branch) and fetches the commit. Before this, the sha was passed to `git clone -b`, which only accepts a branch or a tag.
