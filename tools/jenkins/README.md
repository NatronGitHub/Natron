Natron Continuous Integration Tools
===================================

Scripts used to build and distribute [Natron](http://www.natron.fr) on Linux, Windows and Mac.

Build environments
==================

The Natron 2.6 releases are built in clean environments, not on long-lived build machines:

 * **Linux:** the Rocky Linux 9 Docker image with the Natron SDK, built by `include/scripts/build-Linux-sdk.sh`. See [tools/docker/README.md](../docker/README.md) to build the image and to launch a build in it. On GitHub Actions, `.github/workflows/linux_packages.yml` builds the image and the x86_64 and aarch64 portable archives, and tests them with Natron-Tests.
 * **Windows:** an MSYS2 UCRT64 (or CLANGARM64) environment with the packages of [tools/MINGW-packages](../MINGW-packages/README.md) (`build_installer.yml` sets it up on GitHub Actions).
 * **macOS:** MacPorts with the ports of `tools/MacPorts`, as described in `INSTALL_MACOS.md`, or a clean macOS 14 virtual machine made by [tools/tart](../tart/README.md).

The setup instructions for the INRIA build farm (CentOS 6 machines, the MSYS2 MINGW64/MINGW32 SDK, the `buildmaster.sh` scripts) were removed: they are in the RB-2.5 branch.

Launching a build
=================

`launchBuildMain.sh` does not require any external database, and may be called directly with options from the command line. It is designed to work with the jenkins CI infrastructure.

Example launch command:
```
NATRON_LICENSE=GPL GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=master GIT_COMMIT=f7ee2a2c43bd0de9fc8d3776346408dd4f43426b UNIT_TESTS=true BUILD_NUMBER=1 BUILD_NAME=test COMPILE_TYPE=relwithdebinfo MKJOBS=4 ./launchBuildMain.sh
```

Options
-------

The following options can be passed to `launchBuildMain.sh` as environment variables:

`WORKSPACE`: The absolute path to a directory on the local filesystem were builds are done, external files are downloaded, and builds are archived.

`RELEASE_TAG`: Make a release build. It should be a string "x.y.z" indicating the release number. The corresponding tag is "vx.y.z" in the Natron repository and "Natron-x.y.z" in each of the plugins repository (openfx-io, openfx-misc, openfx-arena, openfx-gmic). Only indicate this to trigger a release.

`SNAPSHOT_BRANCH`: If set and `RELEASE_TAG` is not set, this indicates the branch on which to launch a snapshot from. If `SNAPSHOT_COMMIT` is also set, this will build this specific commit on that branch, otherwise it will build the latest commit on the branch. If `GIT_COMMIT` is set, `GIT_BRANCH` is ignored.

`SNAPSHOT_COMMIT`: If set, this indicate the commit on which to launch a snapshot from. This has to be set in combination with `SNAPSHOT_BRANCH`. If `GIT_COMMIT` is set, `GIT_BRANCH` is ignored.

`UNIT_TESTS`: Run unit tests after build. Value can be "false" or "true" (for compatibility with jenkins).

`NATRON_LICENSE`: Must be `GPL` or `COMMERCIAL`. When GPL is selected, GPL components (such as certain codecs of ffmpeg or demosaic patterns of libraw) will be included. COMMERCIAL builds also leave out openfx-arena's ReadPDF (poppler) and AudioCurve (libsox, which links GPL libraries), and build openfx-gmic with only the G'MIC filters under CeCILL-C. openfx-gmic uses pocketfft (BSD-3-Clause) for its FFT in both licences (openfx-gmic#94): the SDKs don't provide FFTW. `build-plugins.sh` passes the licence to the plug-in repositories that have the option (`-DOFX_LICENSE`: openfx-arena, openfx-gmic, and openfx-misc once it has it; `LICENSE=` for the Makefiles), and openfx-io gets the FFmpeg and LibRaw of the licence. COMMERCIAL builds use an LGPL FFmpeg, a LibRaw without the demosaic packs and a libheif without the x264 and x265 encoders: on Windows the packages `natron_ffmpeg-lgpl`, `natron_libraw-lgpl` and `natron_libheif-lgpl` (`tools/MINGW-packages`), plus `natron_libtiff-lgpl` (MSYS2's libtiff without the GPL JBIG-KIT), on macOS the MacPorts subports `ffmpeg-lgpl`, `libraw-lgpl` and `libheif-lgpl` (`tools/MacPorts`, installed in `$SDK_HOME/libexec`), on Linux COMMERCIAL builds are not done yet, and `launchBuildMain.sh` refuses them (the SDK's libheif links x265 and its OpenImageIO links the GPL LibRaw). On Windows and macOS, the installer scripts (and, on macOS, `build-plugins.sh`) run `check-commercial-bundle.sh`, which fails a COMMERCIAL build if a bundled binary is or links a GPL library, contains an FFmpeg whose licence is not LGPL 2.1 or later, or a LibRaw with a GPL demosaic pack (#1302, #1177). The installers also leave out the Qt modules that are GPL-3 only (Qt Charts, Data Visualization, Network Authorization, Designer, ...) and their PySide modules, which the check rejects too. COMMERCIAL builds are never made in CI, weekly builds or releases (`build_installer.yml` builds GPL only): they are only built by hand, e.g. `tools/tart/natron-build.sh -l COMMERCIAL` on macOS, or `launchBuildMain.sh` with `NATRON_LICENSE=COMMERCIAL` in an MSYS2 UCRT64 shell on Windows. In COMMERCIAL builds, projects that use a left-out node (one of the G'MIC filters that are not under CeCILL-C, ReadPDF, AudioCurve) load without that node. A build of an openfx-gmic without `license.mk` is refused with `PLUGINS_BUILD_SYSTEM=make`, since it would link FFTW.

`DISABLE_BREAKPAD`: If set to 1, natron will be built without crash reporting capabilities.

`COMPILE_TYPE`: The type of build to do, i.e in terms of compilation. Valid values are (`relwithdebinfo`, `release`, `debug`). Must be `relwithdebinfo` or `debug` if DISABLE_BREAKPAD is not 1.

`BITS`: Windows only: Must indicate if this is a 64 or 32 bits build.

`NATRON_DEV_STATUS`: `ALPHA`, `BETA`, `RC`, `STABLE` or `CUSTOM`. This is only useful when doing a release, i.e. if specifying `RELEASE_TAG`.

`NATRON_CUSTOM_BUILD_USER_NAME`: When `NATRON_DEV_STATUS` is `CUSTOM`, tag the build with a specific user/client name that will be displayed within Natron.

`NATRON_BUILD_NUMBER`: When doing a release this is the number of the release (if doing a rebuild).

`NATRON_BUILD_SYSTEM`: `cmake` (the default) or `qmake`. The qmake build is deprecated and is kept on RB-2.6 as a fallback until the CMake-built installers are shown to be equivalent (#1181). The CMake build has no crash reporter yet (#1183), so it implies `DISABLE_BREAKPAD=1`. Linux builds always use `DISABLE_BREAKPAD=1`: the Linux SDK no longer builds Breakpad (`dump_syms`).

`NATRON_EXTRA_CMAKE_FLAGS`: Optional cmake flags to pass when building Natron with CMake, separated by spaces, e.g. `-DNATRON_FAST_MATH=ON`. The options are listed in `Documentation/source/maintainers/building.rst`.

`NATRON_EXTRA_QMAKE_FLAGS`: Optional qmake flags to pass when building Natron with qmake.

`BUILD_NAME`: Set this to label the project build. On the slave, the build artifacts are stored for some amount of time in `$WORKSPACE/builds_archive/$BUILD_NAME/$BUILD_NUMBER`.

`BUILD_NUMBER`: A unique number to identify the build (usually incremented at each build). See also `BUILD_NAME`.

`DISABLE_PORTABLE_ARCHIVE`: If set to 1, a portable archive will not be built (Windows and Linux).

`REMOTE_URL`: The host the build artifacts are uploaded to.
This should be consistent with the `REMOTE_URL` value passed to the upload-artifacts project.

`REMOTE_USER`: The name of the user on `REMOTE_URL` that should be used when uploading using rsync.
This should be consistent with the `REMOTE_USER` value passed to the upload-artifacts project.

`REMOTE_PREFIX`: The prefix to upload artifacts to, relative to `REMOTE_USER@REMOTE_URL`'s home dir.
This should be consistent with the `REMOTE_PREFIX_USER` value passed to the upload-artifacts project.

`DEBUG_SCRIPTS`: If set to 1, binaries from previous build with same options are not cleaned so that the scripts can continue the same compilation.

`PLUGINS_BUILD_SYSTEM`: `cmake` (default) or `make`. With `cmake`, openfx-misc, openfx-io, openfx-arena and openfx-gmic are built with their CMake builds (`build-plugins.sh`, function `ofx_cmake_build`), and a checkout without one is an error. `make` builds them with their Makefiles, the fallback while RB-2.6 is maintained. The CMake builds ignore `OPTFLAG`: they use `-O3` (with `-g` for `relwithdebinfo`); `COMPILE_TYPE=debug` gives `CMAKE_BUILD_TYPE=RelWithDebInfo` with `-O1 -g`, as the Makefiles' `CONFIG=relwithdebinfo OPTFLAG=-O`, so that the macOS bundles are still made self-contained. On macOS, `-DCMAKE_OSX_DEPLOYMENT_TARGET` is `MACOSX_DEPLOYMENT_TARGET` from `common.sh` (14.0 with #1347), as for the Makefiles. On macOS, a release build without OpenMP (Apple clang without MacPorts' `libomp`) is an error. They download the G'MIC sources into `$WORKSPACE/src`, which is kept across builds.

`MAGICK7_PREFIX`: Prefix of the ImageMagick 7 that openfx-arena is built with (its `lib/pkgconfig` is put first in `PKG_CONFIG_PATH` for that build). Defaults to `$SDK_HOME/magick7` on Linux (`include/scripts/pkg/imagemagick7.sh`), `$SDK_HOME/lib/ImageMagick7` on macOS (MacPorts' `ImageMagick7 +natron`, from `tools/MacPorts`), and the MSYS2 prefix on Windows (`natron_imagemagick`, from `tools/MINGW-packages`).

`ARENA_ALLOW_ANY_MAGICK`: `build-plugins.sh` stops if the `Magick++` that pkg-config finds for openfx-arena is not Natron's ImageMagick 7 build (version 7, and a Q32 HDRI `libname` or a prefix in `MAGICK7_PREFIX`). Set to 1 to build openfx-arena with that ImageMagick anyway, e.g. ImageMagick 6 (Natron 2.5's; the output of some plug-ins differs, see openfx-arena's `ChangeLog.md`) or a stock Q16 ImageMagick 7.

`EXTRA_PYTHON_MODULES_SCRIPT`: Path to a python script that should install extra modules with `pip` or `easy_install`.


Upgrading a library on the build machine
=========================================

Linux
-----

The `include/scripts/build-Linux-sdk.sh` script has to be launched. Libraries that already exist with the same version in the `SDK_HOME` will not be built. To decide whether a library should be built or not, the script usually simply tests that a file from the package exists at the beginning of the build section. Top force a rebuild, check in the script to see which file is tested, remove it, and launch the script.

To upgrade a library version, upload the source files to `downloads.natron.fr/Third_Party_Sources`. This is where the scripts has been configured to fetch source files from (but it may also get it from the main site for the given package - this location is a failsafe place). To change this location edit common.sh.
Then in `common.sh`, change the filename of the package to point to the file that was uploaded on the server.

To include a new patch, add it in `include/patches/LIBRARY/patch`. Patch and edit the sdk script to run the patch when needed.

To produce a tarball of the SDK on the build-machine (so it can be re-used somewhere), use the `TAR_SDK=1` option before calling the sdk script;
To also upload the sdk to `downloads.natron.fr/Third_Party_Binaries` you may also provide the option `UPLOAD_SDK=1`.

Windows
-------

The libraries come from MSYS2 and from Natron's own MSYS2 packages: see [tools/MINGW-packages/README.md](../MINGW-packages/README.md).


Mac:
----

See `INSTALL_MACOS.md` in the main Natron repository: libraries are handled by macports. To update a Natron-specific package, the MacPorts `Portfile` located in `Natron/tools/MacPorts` can to be updated.


Jenkins
-------

Launching a build manually on the VM on ci-inria:
Set `DEBUG_SCRIPTS` to 1 to keep all elements built and path checked out so that if re-triggering a jenkins.sh
call attempts to re-use an existing build.
```
export DEBUG_SCRIPTS=1 
JOB_NAME="CustomBuild" BUILD_NUMBER=1 GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=master GIT_COMMIT=08f891ceb8b21ce83eb224f8e035d700310580d2 UNIT_TESTS=true NATRON_LICENSE=GPL ./jenkins.sh
```







