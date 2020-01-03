# Natron-build dockerfile.

FROM natrongithub/natron-sdk:latest as intermediate
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home

COPY *.sh ./
ARG SDK=/opt/Natron-sdk
ARG GCC=$SDK/gcc
ARG FFMPEG=$SDK/ffmpeg-gpl2
ARG LIBRAW=$SDK/libraw-gpl2
ARG OSMESA=$SDK/osmesa
ARG QTDIR=$SDK/qt4

# From launchBuildMain.sh:
# To launch a release, you must set RELEASE_TAG and point GIT_URL to the Natron repo to build from.
# To launch a snapshot, you must set SNAPSHOT_BRANCH and SNAPSHOT_COMMIT and point GIT_URL to the Natron repo to build from.
# To launch a CI (Natron or plug-in repo) you must set GIT_URL, GIT_BRANCH, GIT_COMMIT. GIT_URL may point to either a Natron repository or plug-in repository. The GIT_URL must be known by the script. List of repositories is maintained in gitRepositories.sh
#
# RELEASE_TAG: string "x.y.z" indicating the tag name of the repository to use. Do not add the prefix "tags/", it will be added automatically. Only indicate this to trigger a release.
# SNAPSHOT_BRANCH: If set and RELEASE_TAG is not set, this describes a specific branch to build as a snapshot
# SNAPSHOT_COMMIT: The commit to build for a snapshot on the given SNAPSHOT_BRANCH. If not set, the latest commit of the branch will be built
# UNIT_TESTS: "false" or "true" (for compat with jenkins) to run the full unit tests suite after the build
# NATRON_LICENSE: (GPL or COMMERCIAL) The license to build Natron with. If GPL then GPL components will be included.
# DISABLE_BREAKPAD: If set to 1, natron will be built without crash reporting capabilities
# COMPILE_TYPE: The type of build to do, i.e in terms of compilation. Valid values are (`relwithdebinfo`, `release`, `debug`). Must be `relwithdebinfo` or `debug` if DISABLE_BREAKPAD is not 1.
# NATRON_DEV_STATUS: ALPHA, BETA, RC, STABLE, CUSTOM: this is useful when doing a release (i.e: if specifying RELEASE_TAG.)
# NATRON_CUSTOM_BUILD_USER_NAME: For a release, Tag the build with a specific user/client name
# NATRON_BUILD_NUMBER: When doing a release this is the number of the release (if doing a rebuild)
# NATRON_EXTRA_QMAKE_FLAGS: Optional qmake flags to pass when building Natron
# BUILD_NAME: Give a name to the build so that it can be found in the archive
# DISABLE_RPM_DEB_PKGS: If set to 1, deb and rpm packages will not be built. Otherwise they are only built for a release
# DISABLE_PORTABLE_ARCHIVE: If set to 1, a portable archive will not be built
# BITS: Windows only: Must indicate if this is a 64 or 32 bits build
# DEBUG_SCRIPTS: If set to 1, binaries from previous build with same options are not cleaned so that the scripts can continue the same compilation
# EXTRA_PYTHON_MODULES_SCRIPT: Path to a python script that should install extra modules with pip or easy_install.

ENV GIT_URL=https://github.com/NatronGitHub/Natron.git
ENV GIT_BRANCH=RB-2.3
ENV GIT_COMMIT=
ENV RELEASE_TAG= 
ENV SNAPSHOT_BRANCH=
ENV SNAPSHOT_COMMIT=
ENV UNIT_TESTS=true
ENV NATRON_LICENSE=GPL
ENV DISABLE_BREAKPAD=1
ENV COMPILE_TYPE=release
ENV NATRON_DEV_STATUS=RC
ENV NATRON_CUSTOM_BUILD_USER_NAME=
ENV NATRON_EXTRA_QMAKE_FLAGS=
ENV BUILD_NAME=natron_github_RB2
ENV DISABLE_RPM_DEB_PKGS=1
ENV DISABLE_PORTABLE_ARCHIVE=
ENV BITS=
ENV DEBUG_SCRIPTS=
ENV EXTRA_PYTHON_MODULES_SCRIPT=
ENV BUILD_NUMBER=0

# Already done in the latest SDK images 
#RUN \
#    yum -y install glibc-devel patch mesa-libGL-devel libXrender-devel libSM-devel libICE-devel libX11-devel libXcursor-devel libXrender-devel libXrandr-devel libXinerama-devel libXi-devel libXv-devel libXfixes-devel libXvMC-devel libXxf86vm-devel libxkbfile-devel libXdamage-devel libXp-devel libXScrnSaver-devel libXcomposite-devel libXp-devel libXevie-devel libXres-devel xorg-x11-proto-devel libXxf86dga-devel libdmx-devel libXpm-devel && yum -y clean all

RUN \
    QTDIR="$QTDIR" \
    LIBRARY_PATH="$SDK/lib:$QTDIR/lib:$GCC/lib64:$GCC/lib:$FFMPEG/lib:$LIBRAW/lib:$OSMESA/lib" \
    LD_LIBRARY_PATH="$SDK/lib:$QTDIR/lib:$GCC/lib64:$GCC/lib:$FFMPEG/lib:$LIBRAW/lib" \
    LD_RUN_PATH="$SDK/lib:$QTDIR/lib:$GCC/lib:$FFMPEG/lib:$LIBRAW/lib" \
    CPATH="$SDK/include:$QTDIR/include:$GCC/include:$FFMPEG/include:$LIBRAW/include:$OSMESA/include" \
    PKG_CONFIG_PATH="$SDK/lib/pkgconfig:$OSMESA/lib/pkgconfig:$QTDIR/lib/pkgconfig:$GCC/lib/pkgconfig:$FFMPEG/lib/pkgconfig:$LIBRAW/lib/pkgconfig" \
    PYTHONPATH="$QTDIR/lib/python2.7/site-packages/" \
    PATH="$SDK/bin:$QTDIR/bin:$GCC/bin:$FFMPEG/bin:$LIBRAW_PATH:$PATH" \
    WORKSPACE=/home \
    GIT_URL="$GIT_URL" \
    GIT_BRANCH="$GIT_BRANCH" \
    GIT_COMMIT="$GIT_COMMIT" \
    RELEASE_TAG="$RELEASE_TAG" \
    SNAPSHOT_BRANCH="$SNAPSHOT_BRANCH" \
    SNAPSHOT_COMMIT="$SNAPSHOT_COMMIT" \
    UNIT_TESTS="$UNIT_TESTS" \
    NATRON_LICENSE="$NATRON_LICENSE" \
    DISABLE_BREAKPAD="$DISABLE_BREAKPAD" \
    COMPILE_TYPE="$COMPILE_TYPE" \
    NATRON_DEV_STATUS="$NATRON_DEV_STATUS" \
    NATRON_CUSTOM_BUILD_USER_NAME="$NATRON_CUSTOM_BUILD_USER_NAME" \
    NATRON_EXTRA_QMAKE_FLAGS="$NATRON_EXTRA_QMAKE_FLAGS" \
    BUILD_NAME="$BUILD_NAME" \
    DISABLE_RPM_DEB_PKGS="$DISABLE_RPM_DEB_PKGS" \
    DISABLE_PORTABLE_ARCHIVE="$DISABLE_PORTABLE_ARCHIVE" \
    BITS="$BITS" \
    DEBUG_SCRIPTS="$DEBUG_SCRIPTS" \
    EXTRA_PYTHON_MODULES_SCRIPT="$EXTRA_PYTHON_MODULES_SCRIPT" \
    BUILD_NUMBER="$BUILD_NUMBER" \    
    MKJOBS=3 ./launchBuildMain.sh
RUN rm -rf /home/tmp

FROM centos:6
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home
COPY --from=intermediate /home /home
CMD runUnitTests.sh
#COPY --from=intermediate /home/src /opt/Natron-sdk/src
## retrieve sources using:
## docker run natrongithub/natron-sdk:latest tar -C /opt/Natron-sdk -cf - src | tar xvf -
