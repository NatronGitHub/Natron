#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****
#
# Build Natron SDK (CY2015+) for Linux
#
# By default, running this script will rebuild all packages that need to be updated.
# Some dependencies are automatically handled, e.g. if package B depends on A, B
# will be rebuilt when A is rebuilt.
#
# Options available by setting the following environment variables:
#
# GEN_DOCKERFILE=1 : Output a Dockerfile rather than building the SDK.
# LIST_STEPS=1 : Output an ordered list of steps to build the SDK
# LAST_STEP=step_name : Only build until this step (be careful, as dependent packages are not rebuilt).
# FORCE_BUILD=1 : Force build, even if up-to-date. If LAST_STEP is specified, only force-build this step.
# DOWNLOAD=1 : build nothing, just download the sources
#
# Only available when GEN_DOCKERFILE, LIST_STEPS and LAST_STEPS are not set:
#
# TAR_SDK=1 : Make an archive of the SDK when building is done and store it in $SRC_PATH
# UPLOAD_SDK=1 : Upload the SDK tar archive to $REPO_DEST if TAR_SDK=1
#
# ROCKY=9|8 : the Rocky Linux major version (default: 9, the Natron 2.6 baseline). The SDK is
# built on Rocky Linux only (the CentOS and Ubuntu SDKs are in the RB-2.5 branch).

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

PKGOS=Linux

ROCKY="${ROCKY:-9}"
case "$ROCKY" in
    8|9) ;;
    *) (>&2 echo "Error: unsupported ROCKY=$ROCKY (the SDK is built on Rocky Linux 8 or 9)"); exit 1 ;;
esac

#############################
# Package-specific switches #
#############################
# WITH_MARIADB=true|false build MariaDB/MySQL as well as the corresponding Qt and Python components
WITH_MARIADB="${WITH_MARIADB:-true}"

# GCC_TOOLSET=N gcc-toolset version used on Rocky Linux 8 and 9 (default: 14, i.e. GCC 14.2,
# as in the VFX Reference Platform CY2026/CY2027)
GCC_TOOLSET="${GCC_TOOLSET:-14}"
# libclang used by the shiboken2 generator on Rocky Linux (see the Dockerfile generation below).
# Shiboken2 5.15.19, even with the libclang >= 16 patches, misparses some default arguments with
# libclang 22 (e.g. QEvent::Type values resolved in the wrong class): use an older libclang.
if [ "${ROCKY}" -ge 9 ]; then
    # SHIBOKEN2_CLANG_VERSION=N: major version of EPEL's clangN-devel compat package
    SHIBOKEN2_CLANG_VERSION="${SHIBOKEN2_CLANG_VERSION:-19}"
    SHIBOKEN2_CLANG_PKGS="clang${SHIBOKEN2_CLANG_VERSION}-devel llvm${SHIBOKEN2_CLANG_VERSION}-devel"
    SHIBOKEN2_LLVM_DIR="/usr/lib64/llvm${SHIBOKEN2_CLANG_VERSION}"
else
    # SHIBOKEN2_CLANG_VERSION=X.Y.Z: version of AppStream's clang-devel (as in MacPorts)
    SHIBOKEN2_CLANG_VERSION="${SHIBOKEN2_CLANG_VERSION:-18.1.8}"
    SHIBOKEN2_CLANG_PKGS="clang-devel-${SHIBOKEN2_CLANG_VERSION} llvm-devel-${SHIBOKEN2_CLANG_VERSION}"
    SHIBOKEN2_LLVM_DIR="/usr"
fi
if [ "${DOWNLOAD:-}" = "1" ] && [ -z "${GEN_DOCKERFILE+x}" ]; then
    echo "*** downloading all source distributions"
    if [ -z "${WORKSPACE+x}" ]; then
        (>&2 echo "Error: set WORKSPACE. Source distributions will be downloaded to WORKSPACE/src")
        exit 1
    fi
    if [ ! -d "${WORKSPACE}" ]; then
        (>&2 echo "Error: directory WORKSPACE/src=$WORKSPACE/src does not exist")
        exit 1
    fi
fi
source common.sh

newdists=()
newdistsurls=()

scriptdir="$(dirname "$0")"

if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${GEN_DOCKERFILE:-}" = "2" ]; then
    cat <<EOF
# Natron-SDK dockerfile.
#
# The natron-sdk docker image should be created using the following commands:
# cd tools/docker/natron-sdk-rockylinux/
# ./build.sh
# SDK is installed in $SDK_HOME
EOF
    DOCKER_ENV="ENV QTDIR=\"\$QTDIR\" \\
    LIBRARY_PATH=\"\$SDK/lib:\$QTDIR/lib:\$FFMPEG/lib:\$LIBRAW/lib:\$OSMESA/lib\" \\
    LD_LIBRARY_PATH=\"\$SDK/lib:\$QTDIR/lib:\$FFMPEG/lib:\$LIBRAW/lib\" \\
    LD_RUN_PATH=\"\$SDK/lib:\$QTDIR/lib:\$FFMPEG/lib:\$LIBRAW/lib\" \\
    CPATH=\"\$SDK/include:\$QTDIR/include:\$FFMPEG/include:\$LIBRAW/include:\$OSMESA/include\" \\
    PKG_CONFIG_PATH=\"\$SDK/lib/pkgconfig:\$OSMESA/lib/pkgconfig:\$QTDIR/lib/pkgconfig:\$FFMPEG/lib/pkgconfig:\$LIBRAW/lib/pkgconfig\" \\
    PATH=\"\$SDK/bin:\$QTDIR/bin:\$FFMPEG/bin:\$LIBRAW_PATH:\$PATH\" \\
    WORKSPACE=/home \\
    GIT_URL=https://github.com/NatronGitHub/Natron.git \\
    GIT_BRANCH=RB-2.6 \\
    GIT_COMMIT= \\
    RELEASE_TAG=  \\
    SNAPSHOT_BRANCH= \\
    SNAPSHOT_COMMIT= \\
    UNIT_TESTS=true \\
    NATRON_LICENSE=GPL \\
    DISABLE_BREAKPAD=1 \\
    COMPILE_TYPE=release \\
    NATRON_DEV_STATUS=RC \\
    NATRON_CUSTOM_BUILD_USER_NAME= \\
    NATRON_BUILD_SYSTEM=cmake \\
    NATRON_EXTRA_CMAKE_FLAGS= \\
    NATRON_EXTRA_QMAKE_FLAGS= \\
    BUILD_NAME=natron_github_RB2 \\
    DISABLE_PORTABLE_ARCHIVE= \\
    BITS= \\
    DEBUG_SCRIPTS= \\
    EXTRA_PYTHON_MODULES_SCRIPT= \\
    BUILD_NUMBER=0"

    BUILD_LINUX_SDK="ROCKY=${ROCKY} ./build-Linux-sdk.sh"

    # rockylinux/rockylinux images are multi-arch (amd64, arm64): the
    # architecture is the one of the image (docker build --platform),
    # and common.sh takes it from uname -m inside the container.
    DOCKER_BASE="rockylinux/rockylinux:${ROCKY}"
    PREYUM=
    ARCH=
    DTSYUM=
    DTSSDK=
    # Enable powertools on Rocky Linux 8, CRB on Rocky Linux >= 9
    # https://www.how2shout.com/linux/how-to-enable-powertools-repository-on-rocky-linux-8/
    EL_DEVEL_REPO=powertools
    if [ "${ROCKY}" -ge 9 ]; then
        EL_DEVEL_REPO=crb
    fi
    DTSYUM+="dnf -y install dnf-plugins-core && dnf -y install epel-release && dnf config-manager --set-enabled ${EL_DEVEL_REPO} && dnf -y update && dnf -y groupinstall 'Development Tools' && "
    # The system GCC of Rocky Linux 8 (8.5) has incomplete C++17 and no C++20 support,
    # and the one of Rocky Linux 9 (11.5) has incomplete C++20 support: use the
    # gcc-toolset (GCC 14.2, as in the VFX Reference Platform CY2026/CY2027),
    # available for x86_64 and aarch64.
    # libclang, for the shiboken2 generator (see pkg/pyside2.sh), which also needs
    # LLVM_INSTALL_DIR to find clang's builtin headers (see SHIBOKEN2_CLANG_VERSION).
    DTSYUM+="dnf -y install gcc-toolset-${GCC_TOOLSET} ${SHIBOKEN2_CLANG_PKGS} && "
    if [ "${SHIBOKEN2_LLVM_DIR}" != /usr ]; then
        # shiboken2 looks for the builtin headers in $LLVM_INSTALL_DIR/lib/clang/<version>/include,
        # but EPEL's clangN-resource-filesystem puts them in /usr/lib/clang/N/include
        DTSYUM+="ln -sfn /usr/lib/clang ${SHIBOKEN2_LLVM_DIR}/lib/clang && "
    fi
    DOCKER_ENV_TOOLSET="ENV PATH=\"/opt/rh/gcc-toolset-${GCC_TOOLSET}/root/usr/bin:\$PATH\" \\
    LLVM_INSTALL_DIR=${SHIBOKEN2_LLVM_DIR}"
    # libXvMC was removed in EL9
    YUM_XVMC=libXvMC-devel
    if [ "${ROCKY}" -ge 9 ]; then
        YUM_XVMC=
        # Rust (cargo, rustc), to build librsvg >= 2.41 (see pkg/librsvg.sh)
        DTSYUM+="dnf -y install rust-toolset && "
        # doxygen (CRB): the Python bindings of libxml2 >= 2.14, needed by itstool, are
        # generated from its doxygen XML (see pkg/libxml2.sh)
        DTSYUM+="dnf -y install doxygen && "
    fi
    # The xcb platform plug-in of Qt 5.15 (pkg/qt5.sh) needs libxcb with its extensions
    # (randr, render, shape, shm, sync, xfixes, xinerama, xkb, xinput, glx), the xcb-util
    # libraries (icccm/wm, image, keysyms, renderutil), libX11-xcb and libxkbcommon-x11.
    YUM_XCB="libxcb-devel xcb-util-devel xcb-util-wm-devel xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel libxkbcommon-x11-devel"
    # Note: perl-version was added for qt4webkit, see https://github.com/NatronGitHub/Natron/issues/351#issuecomment-524068232
    # perl-Encode and perl-Data-Dumper needed to build texinfo before perl
    YUM_PKGS="ca-certificates make util-linux git tar bzip2 wget glibc-devel diffutils patch zip unzip alsa-lib-devel libX11-devel mesa-libGL-devel mesa-libEGL-devel mesa-libGLU-devel libXrender-devel libSM-devel libICE-devel libXcursor-devel libXrender-devel libXrandr-devel libXinerama-devel libXi-devel libXv-devel libXfixes-devel ${YUM_XVMC} libXxf86vm-devel libxkbcommon-devel ${YUM_XCB} libxkbfile-devel libXdamage-devel libXp-devel libXScrnSaver-devel libXcomposite-devel libXp-devel libXres-devel libXtst-devel xorg-x11-proto-devel libXxf86dga-devel libXpm-devel perl-Digest-MD5 perl-version perl-Encode perl-Data-Dumper"
    SDKPREP="RUN ${PREYUM}${DTSYUM}yum -y install ${YUM_PKGS} && yum -y clean all"
    cat <<EOF
FROM $DOCKER_BASE as intermediate
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home
ARG SDK=$SDK_HOME
${ARCH:+ARG ARCH=$ARCH
}RUN ${PREYUM}${DTSYUM}\\
    yum -y install gcc gcc-c++ ${YUM_PKGS} && \\
    yum clean all
COPY include/patches/ include/patches/
COPY include/scripts/build-Linux-sdk.sh common.sh compiler-common.sh ./
ENV WORKSPACE=/home
EOF

fi

function dobuild ()
{
    if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${GEN_DOCKERFILE:-}" = "2" ] || [ "${LIST_STEPS:-}" = "1" ] || [ "${DOWNLOAD:-}" = "1" ]; then
        return 1
    fi
    return 0 # must return a status
}

function build()
{
    step="$1"
    if [ -f "${scriptdir}/pkg/${step}.sh" ]; then
        . "${scriptdir}/pkg/${step}.sh"
    else
        if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${GEN_DOCKERFILE:-}" = "2" ] || [ "${LIST_STEPS:-}" = "1" ] || [ "${step}" = "${LAST_STEP:-}" ]; then
            (>&2 echo "Error: package file ${scriptdir}/pkg/${step}.sh not available")
            exit 1
        fi
    fi
}

function checkpoint()
{
    if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${GEN_DOCKERFILE:-}" = "2" ]; then
        if [ "$step" = "$checkpointstep" ]; then
            # checkpoint was walled twice in a row
            return 0
        fi
        copyline="COPY "
        for s in in "${pkgs[@]}"; do
            if [ -f "$scriptdir/pkg/${s}.sh" ]; then
                copyline="$copyline include/scripts/pkg/${s}.sh "
            fi
            checkpointstep="$s"
        done
        echo "$copyline pkg/"
        echo "RUN ${DTSSDK:-}env LAST_STEP=$checkpointstep $BUILD_LINUX_SDK || (cd \$SDK/var/log/Natron-Linux-*-SDK/ && cat "'`ls -t |grep -e '"'\.log$'"'|head -1`'" && false)"
        pkgs=()
    fi
    return 0
}

# should we build this step?
# Also prints output if GEN_DOCKERFILE=1 or LIST_STEPS=1
function build_step ()
{
    # no-build cases (we avoid printing the same step twice)
    if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${GEN_DOCKERFILE:-}" = "2" ]; then
        if [ "$step" != "$prevstep" ]; then
            # push to the list of packages
            pkgs+=("$step")
        fi
        prevstep="$step"
        if [ "${GEN_DOCKERFILE:-}" = "2" ]; then
            # special value of GEN_DOCKERFILE that prints all the steps
            checkpoint
        fi
        return 1 # do not build
    fi
    if [ "${LIST_STEPS:-}" = "1" ]; then
        if [ "$step" != "$prevstep" ]; then
            echo "$step"
        fi
        prevstep="$step"
        return 1
    fi
    # maybe-build cases
    if dobuild && [ -z ${LAST_STEP+x} ]; then # LAST_STEP is not set, so build
        prevstep="$step"
        return 0
    fi
    if dobuild && [ "$reachedstep" = "0" ] ; then # build this step
        if [ "${LAST_STEP:-}" = "$step" ]; then # this is the last step to build
            reachedstep=1
        fi
        prevstep="$step"
        return 0
    fi
    if dobuild && [ "$prevstep" = "$step" ]; then
        # build_step is called a second time on LAST_STEP, this is OK
        return 0
    fi
    prevstep="$step"
    return 1 # must return a status
}

# should we download sources for this step?
# Also prints output if GEN_DOCKERFILE=1 or LIST_STEPS=1
function download_step ()
{
    if build_step; then
        return 0 # must download to build
    fi
    if [ "${DOWNLOAD:-}" = "1" ]; then
        return 0 # download everything, build nothing
    fi
    return 1
}

# is build forced?
function force_build()
{
    # for build if:
    # - FORCE_BUILD is 1 and LAST_STEP is not set
    # or
    # - FORCE_BUILD is 1 and LAST_STEP is the current step
    if [ "${FORCE_BUILD:-}" = "1" ] && [ -z ${LAST_STEP+x} -o "${LAST_STEP:-}" = "$step" ]; then
        return 0
    fi
    return 1 # must return a status
}

if dobuild; then
    if [ "${DEBUG:-}" = "1" ]; then
        export CMAKE_BUILD_TYPE="Debug"
    else
        export CMAKE_BUILD_TYPE="Release"
    fi

    error=false
    # Check that mandatory utilities are present
    for e in gcc g++ make wget tar patch find gzip; do
        if ! type -p "$e" > /dev/null; then
            (>&2 echo "Error: $e not available")
            error=true
        fi
    done

    if [ ! -f /usr/include/X11/Xlib.h ] && [ ! -f /usr/X11R6/include/X11/Xlib.h ]; then
        (>&2 echo "Error: X11/Xlib.h not available (on Rocky Linux, do 'dnf install alsa-lib-devel libICE-devel libSM-devel libX11-devel libXScrnSaver-devel libXcomposite-devel libXcursor-devel libXdamage-devel libXfixes-devel libXi-devel libXinerama-devel libXp-devel libXp-devel libXpm-devel libXrandr-devel libXrender-devel libXres-devel libXtst-devel libXv-devel libXvMC-devel libXxf86dga-devel libXxf86vm-devel libxkbfile-devel mesa-libGL-devel mesa-libEGL-devel mesa-libGLU-devel')")
        error=true
    fi

    if $error; then
        (>&2 echo "Error: build cannot proceed, exiting.")
        exit 1
    fi

    # Check distro and version

    if [ "$PKGOS" = "Linux" ]; then
        if [ ! -s /etc/redhat-release ]; then
            (>&2 echo "Warning: Build system has been designed for Rocky Linux (RHEL), use at OWN risk!")
            sleep 5
        else
            RHEL_MAJOR=$(tr -dc '0-9.'< /etc/redhat-release|cut -d \. -f1)
            RHEL_MINOR=$(tr -dc '0-9.'< /etc/redhat-release|cut -d \. -f2)
            RHEL_VERSION="${RHEL_MAJOR}.${RHEL_MINOR}"
            if [ "${RHEL_MAJOR}" != "8" ] && [ "${RHEL_MAJOR}" != "9" ]; then
                (>&2 echo "Warning: Wrong RHEL version (${RHEL_VERSION}): only Rocky Linux 8 and 9 are supported (the Natron 2.6 releases are built on Rocky Linux 9).")
            fi
        fi
    fi


    BINARIES_URL="$REPO_DEST/Third_Party_Binaries"


    SDK="Linux-$ARCH-SDK"

    if [ -z "${MKJOBS:-}" ]; then
        #Default to 4 threads
        MKJOBS=$DEFAULT_MKJOBS
    fi
fi # dobuild

# see https://stackoverflow.com/a/24067243
function version_gt() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }

# $1: file, $2: expected sha256 (if empty, the file is not checked)
function sha256_ok() {
    local file=$1
    local expected=${2:-}
    if [ -z "$expected" ]; then
        return 0
    fi
    local actual
    actual=$(sha256sum "$file" | cut -d' ' -f1)
    if [ "$actual" != "$expected" ]; then
        (>&2 echo "*** $file: sha256 is $actual, expected $expected")
        return 1
    fi
    return 0
}

# $1: URL, $2: destination file, $3: expected sha256 (may be empty)
# Downloads with certificate checking, and removes the file if the download or the checksum fails.
function fetch_checked() {
    local url=$1
    local dest=$2
    local sha256=${3:-}
    local setcertdir=""
    if [ -d /etc/ssl/certs ]; then
        # or wget fails on Ubuntu for sourceforge and all letsencrypt-certified sites
        setcertdir="env SSL_CERT_DIR=/etc/ssl/certs"
    fi
    echo "*** downloading $url"
    # wget is the system one: don't let it load the SDK's libraries (on Rocky Linux 9, the
    # system gnutls fails with the SDK's older nettle)
    if $setcertdir env -u LD_LIBRARY_PATH wget -q "$url" -O "$dest" && [ -s "$dest" ] && sha256_ok "$dest" "$sha256"; then
        return 0
    fi
    echo "*** downloading $url failed!"
    rm -f "$dest" > /dev/null 2>&1 || true
    return 1
}

# $1: master site, $2: package (file name), $3: expected sha256 of the file (optional, but every
# package should have one). The file is first fetched from the Natron mirror ($THIRD_PARTY_SRC_URL).
function download() {
    if dobuild || [ "${DOWNLOAD:-}" = "1" ]; then
        local master_site=$1
        local package=$2
        local sha256=${3:-}
        if [ -s "$SRC_PATH/$package" ] && ! sha256_ok "$SRC_PATH/$package" "$sha256"; then
            echo "*** removing $SRC_PATH/$package (wrong checksum)"
            rm -f "$SRC_PATH/$package"
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            if [ "${DOWNLOAD:-}" = "1" ]; then
                # print list of new dists, in case subsequent download hangs
                if [ "${#newdists[*]}" -gt 0 ]; then
                    echo "*** newdists = ${newdists[*]}"
                fi
                if [ "${#newdistsurls[*]}" -gt 0 ]; then
                    echo "*** newdistsurls = ${newdistsurls[*]}"
                fi
            fi
            if ! fetch_checked "$THIRD_PARTY_SRC_URL/$package" "$SRC_PATH/$package" "$sha256"; then
                if fetch_checked "$master_site/$package" "$SRC_PATH/$package" "$sha256"; then
                    newdists+=("$package")
                    newdistsurls+=("$master_site/$package")
                fi
            fi
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            (>&2 echo "Error: unable to download $package.")
            exit 1
        fi
    fi
}

# $1: GitHub user, $2: repository, $3: version, $4: tag prefix, $5: package (file name),
# $6: expected sha256 of the file (optional, but every package should have one)
function download_github() {
    if dobuild || [ "${DOWNLOAD:-}" = "1" ]; then
        # e.g.:
        #download_github uclouvain openjpeg 2.3.0 v openjpeg-2.3.0.tar.gz <sha256>
        #download_github OpenImageIO oiio 1.6.18 tags/Release- oiio-1.6.18.tar.gz <sha256>
        local user=$1
        local repo=$2
        local version=$3
        local tagprefix=$4
        local package=$5
        local sha256=${6:-}
        local url="https://github.com/${user}/${repo}/archive/${tagprefix}${version}.tar.gz"
        if [ -s "$SRC_PATH/$package" ] && ! sha256_ok "$SRC_PATH/$package" "$sha256"; then
            echo "*** removing $SRC_PATH/$package (wrong checksum)"
            rm -f "$SRC_PATH/$package"
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            if ! fetch_checked "$THIRD_PARTY_SRC_URL/$package" "$SRC_PATH/$package" "$sha256"; then
                if fetch_checked "$url" "$SRC_PATH/$package" "$sha256"; then
                    newdists+=("$package")
                    newdistsurls+=("$url")
                fi
            fi
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            (>&2 echo "Error: unable to download $package")
            exit 1
        fi
    fi
}

# see https://stackoverflow.com/a/43136160/2607517
# $1: git repo
# $2: commit SHA1
function git_clone_commit() {
    if dobuild; then
        local repo=${1##*/}
        repo=${repo%.git}
        git clone -n "$1" && ( cd "$repo" && git checkout "$2" )
    fi
}

function git_clone_branch_or_tag() {
    if dobuild; then
        local repo=${1##*/}
        repo=${repo%.git}
        git clone --depth 1 -b "$2" "$1"
    fi
}

function untar() {
    if dobuild; then
        echo "*** extracting $1"
        # --no-same-owner: some tarballs have uids that are not mapped in rootless
        # docker/podman user namespaces ("Cannot change ownership to uid ...")
        tar --no-same-owner -xf "$1"
    fi
}

function start_build() {
    if dobuild; then
        echo "*** Building $step..."
        touch "$LOGDIR/$step".stamp
        exec 3>&1 4>&2 1>"$LOGDIR/$step".log 2>&1
        pushd "$TMP_PATH"
    fi
}

function end_build() {
    if dobuild; then
        popd #"$TMP_PATH"
        find "$SDK_HOME" -newer "$LOGDIR/$step".stamp > "$LOGDIR/$step"-files.txt
        rm "$LOGDIR/$step".stamp
        exec 1>&3 2>&4
        echo "*** Done"
    fi
}

if dobuild; then
    echo 
    echo "Building Natron-$SDK with $MKJOBS threads ..."
    echo

    if [ -d "$TMP_PATH" ]; then
        rm -rf "$TMP_PATH"
    fi
    mkdir -p "$TMP_PATH"
    if [ ! -d "$SRC_PATH" ]; then
        mkdir -p "$SRC_PATH"
    fi

    # check for dirs
    if [ ! -d "$SDK_HOME/include" ]; then
        # see http://www.linuxfromscratch.org/lfs/view/development/chapter08/creatingdirs.html
        mkdir -pv "$SDK_HOME"/{bin,include,lib,sbin,src}
        mkdir -pv "$SDK_HOME"/share/{color,dict,doc,info,locale,man}
        mkdir -v  "$SDK_HOME"/share/{misc,terminfo,zoneinfo}
        mkdir -v  "$SDK_HOME"/libexec
        mkdir -pv "$SDK_HOME"/share/man/man{1..8}
        case $(uname -m) in
            x86_64|aarch64|ppc64le) ln -sfv lib "$SDK_HOME"/lib64 ;;
        esac
    fi

    LOGDIR="$SDK_HOME/var/log/Natron-$SDK"
    if [ ! -d "$LOGDIR" ]; then
        mkdir -p "$LOGDIR"
    fi
    echo "*** Info: Build logs are in $LOGDIR"
fi

step="" # the last package built (none yet)
checkpointstep=""
prevstep=""
reachedstep=0 # becomes 1 when LAST_STEP is reached
pkgs=()

checkpoint

# Setup env

if dobuild; then
    # set failsafe values for variables that are either
    # understood by configure/cmake scripts or by GCC itself
    export CMAKE_PREFIX_PATH="$SDK_HOME"
    #export CPPFLAGS="-I$SDK_HOME/include" # configure/cmake
    #export LDFLAGS="-L$SDK_HOME/lib" # configure/cmake
    export CPATH="$SDK_HOME/include" # gcc/g++'s include path
    export LIBRARY_PATH="$SDK_HOME/lib" # gcc/g++'s library path

    export QT5PREFIX="$SDK_HOME/qt5"

    export PATH="$SDK_HOME/bin:$PATH"
    export LD_LIBRARY_PATH="$SDK_HOME/lib:$QT5PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    #export LD_RUN_PATH="$LD_LIBRARY_PATH"
    #PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig"
    BOOST_ROOT="$SDK_HOME"
    OPENJPEG_HOME="$SDK_HOME"
    THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    export PKG_CONFIG_PATH LD_LIBRARY_PATH PATH BOOST_ROOT OPENJPEG_HOME THIRD_PARTY_TOOLS_HOME PYTHON_HOME
fi

# gcc-toolset-${GCC_TOOLSET} (installed with dnf, see the Dockerfile generation above)
if dobuild; then
    export LLVM_INSTALL_DIR="${LLVM_INSTALL_DIR:-$SHIBOKEN2_LLVM_DIR}"
fi
DTSSDK="source scl_source enable gcc-toolset-${GCC_TOOLSET} && "
if dobuild; then
    case " ${X_SCLS:-} " in
        *" gcc-toolset-${GCC_TOOLSET} "*)
            ;;
        *)
            # not enabled yet (e.g. this script is run outside of the Dockerfile)
            if [ -f "/opt/rh/gcc-toolset-${GCC_TOOLSET}/enable" ]; then
                set +u
                source "/opt/rh/gcc-toolset-${GCC_TOOLSET}/enable"
                set -u
            fi
            ;;
    esac
fi
if dobuild; then
    GCC_VERSION=$(gcc -dumpfullversion)
    if version_gt "$GCC_VERSION" 13.99; then
        # GCC 14 turned obsolete C constructs into errors (implicit function
        # declarations, implicit int, int-conversion, incompatible pointer types),
        # and many of the older packages of the SDK still have some, e.g. libffi 3.4.4
        # on aarch64. Make them warnings again. These options are C-only: g++ just
        # prints a warning when they appear in CXXFLAGS.
        GCC14_CFLAGS="-Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"
        BF="$BF $GCC14_CFLAGS"
    fi
else
    GCC_VERSION="${GCC_TOOLSET}"
fi
BOOTSTRAP_GCC_VERSION=$GCC_VERSION

build bzip2
build xz # (required to uncompress source tarballs)
build m4
build ncurses # (required for readline)
build texinfo # (required for bison)
build bison # (for SeExpr)
build flex # (for SeExpr)
build pkg-config
build libtool
build expat # (for perl)
build gperf # (used by fontconfig) as well as assemblers (yasm and nasm)
build perl # (required to install XML:Parser perl module used by intltool)
build autoconf
build automake
build yasm
build nasm # (for x264, lame, and others)
build gmp # (for nettle's hogweed, used by gnutls)
build openssl
build patchelf
build gettext
build intltool
build zlib
build zstd # (for tiff, curl, libarchive, boost_iostreams, Qt and others: they would link the system libzstd otherwise)

checkpoint

build readline
build nettle # (for gnutls)
build libtasn1 # (for gnutls)
build libunistring # (for gnutls)
build libffi
build p11-kit # (for gnutls)
build gnutls # (for ffmpeg)
build curl
build libarchive # (for cmake)
#build libuv # (for cmake but we use the one bundled with cmake)
build cmake
build libzip # (requires cmake)
#build berkeleydb # (optional for python3)
build sqlite # (required for the QtSql SQLite module, optional for python3)
build pcre # (required by glib)
build pcre2 # (required by mariadb)

checkpoint

build git # (requires curl and pcre)

checkpoint

if $WITH_MARIADB; then
    build mariadb # (for the Qt mariadb plugin and the python mariadb adapter)
fi
build postgresql # (for the Qt postgresql plugin and the python postgresql adapter)

checkpoint

build python3

checkpoint

build ninja # (requires python3, required for building glib > 2.59.0)
build meson # (requires python3, required for building glib > 2.59.0)

checkpoint

if $WITH_MARIADB; then
    build mysqlclient # (MySQL/MariaDB connector for python 3)
fi
build psycopg2 # (PostgreSQL connector for python 3)
build sphinx # (Python documentation generator)
build sphinx-rtd-theme # (Read the Docs theme for Sphinx)

checkpoint

build icu # requires python

checkpoint

build osmesa

checkpoint

build libpng
build freetype
build libmount # (required by glib)
build fontconfig

checkpoint

build libxml2
build libxslt # (required by glib)
build dbus # (for QtDBus and glib)
build glib
build itstool # (required by shared-mime-info)
build boost

checkpoint

build cppunit
build libjpeg-turbo
build giflib
build tiff
build jasper
build lcms2
build librevenge
build libcdr
build openjpeg
build libraw

checkpoint

build ilmbase
build openexr
build pixman
build cairo
build harfbuzz
build fribidi # (for libass and ffmpeg)
build pango
build libcroco # (requires glib and libxml2)
build xmlto # (required by shared-mime-info)
build shared-mime-info # (required by gdk-pixbuf)
build gdk-pixbuf
build librsvg # (without vala support)

checkpoint

build x265 # (for ffmpeg and libheif)
build libde265 # (for libheif)
build dav1d # (for ffmpeg and libheif)
build aom # (for ffmpeg and libheif)
build libheif # (for imagemagick and openimageio)

checkpoint

build imagemagick6
build imagemagick7
build glew
build poppler-glib # (without curl, nss3, qt4, qt5)
build poppler-data
build tinyxml # (used by OpenColorIO)
build yaml-cpp # (0.5.3 requires boost, 0.6+ requires C++11, used by OpenColorIO)
build opencolorio1 # (uses yaml-cpp, tinyxml, lcms)
build opencolorio # (uses yaml-cpp, tinyxml, lcms)
build webp # (for OIIO)
build openimageio
build seexpr211
build seexpr
#build eigen
#build opencv

checkpoint

build lame
build libogg
build libvorbis
build libtheora

checkpoint

build flac # (for libsndfile, sox)
build libsndfile # (for sox)
build libmad # (for sox)
build libmodplug
build libvpx
build speex # (EOL, use opus)
build opus
build opusfile # (for sox)
build sox
build orc
build x264
build xvid
build soxr
build libass # (for ffmpeg)
build libbluray # (for ffmpeg)
build openh264 # (for ffmpeg)
build snappy # (for ffmpeg)
checkpoint
build ffmpeg

checkpoint

build qt5

checkpoint

build pyside2

if dobuild; then
    # make sure all libs are user-writable and executable
    for subdir in . ffmpeg-gpl2 ffmpeg-lgpl libraw-gpl2 libraw-lgpl; do
        if compgen -G "$SDK_HOME/$subdir/lib*/lib*.so*" > /dev/null; then
            chmod 755 "$SDK_HOME"/$subdir/lib*/lib*.so*
        fi
    done

    if [ -n "${TAR_SDK:-}" ]; then
        # Done, make a tarball
        pushd "$SDK_HOME/.."
        tar cJf "$SRC_PATH/Natron-$SDK.tar.xz" "Natron-sdk"
        echo "*** SDK available at $SRC_PATH/Natron-$SDK.tar.xz"

        if [ -n "${UPLOAD_SDK:-}" ]; then
            rsync -avz -O --progress --verbose -e 'ssh -oBatchMode=yes' "$SRC_PATH/Natron-$SDK.tar.xz" "$BINARIES_URL"
        fi
        popd
    fi

    echo
    echo "Check for broken libraries and binaries... (everything is OK if nothing is printed)"
    for subdir in . ffmpeg-gpl2 ffmpeg-lgpl libraw-gpl2 libraw-lgpl qt5; do
        [ -d "$SDK_HOME/$subdir/lib" ] && LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/lib" -name '*.so' -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
        [ -d "$SDK_HOME/$subdir/bin" ] && LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/bin" -maxdepth 1 -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
    done
    echo "Check for broken libraries and binaries... done!"

    echo
    echo "Natron SDK Done"
    echo
fi

checkpoint

if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${GEN_DOCKERFILE:-}" = "2" ]; then
    SDK_DEFAULT_QT=qt5
    cat <<EOF
RUN xz $SDK_HOME/var/log/Natron-Linux-*-SDK/*
FROM $DOCKER_BASE
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home
COPY --from=intermediate $SDK_HOME $SDK_HOME
ARG SDK=$SDK_HOME
ARG QTDIR=\$SDK/${SDK_DEFAULT_QT}
ARG FFMPEG=\$SDK/ffmpeg-gpl2
ARG LIBRAW=\$SDK/libraw-gpl2
ARG OSMESA=\$SDK/osmesa
$SDKPREP
$DOCKER_ENV
${DOCKER_ENV_TOOLSET:-}

COPY \\
    common.sh \\
    compiler-common.sh \\
    launchBuildMain.sh \\
    manageBuildOptions.sh \\
    manageLog.sh \\
    createBuildOptionsFile.sh \\
    gitRepositories.sh \\
    checkout-repository.sh \\
    build-plugins.sh \\
    build-natron.sh \\
    build-Linux-installer.sh \\
    gen-natron-doc.sh \\
    zip-python.sh \\
    runUnitTests.sh \\
    uploadArtifactsMain.sh \\
    ./
COPY include/ include/
#COPY --from=intermediate /home/src /opt/Natron-sdk/src
## retrieve sources using:
## docker run natrongithub/natron-sdk:latest tar -C /opt/Natron-sdk -cf - src | tar xvf -
EOF
    # gcc-toolset-${GCC_TOOLSET} is also first in PATH (see ENV above), so that
    # "docker run <image> bash -c ..." uses it too.
    DTSENABLE="scl enable gcc-toolset-${GCC_TOOLSET} "
    cat <<EOF
CMD ${DTSENABLE}launchBuildMain.sh
EOF
fi

if [ -z "${GEN_DOCKERFILE+x}" ]; then
    if [ "${#newdists[*]}" -gt 0 ]; then
        echo "*** List of packages that were not available in $THIRD_PARTY_SRC_URL:"
        echo "${newdists[*]}"
    else
        echo "*** Note: All packages were available from $THIRD_PARTY_SRC_URL"
    fi
    if [ "${#newdistsurls[*]}" -gt 0 ]; then
        echo "*** URLs of packages that were not available in $THIRD_PARTY_SRC_URL:"
        echo "${newdistsurls[*]}"
    fi
fi
exit 0

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:

