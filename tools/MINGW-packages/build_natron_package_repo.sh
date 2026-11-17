#!/bin/sh
set -e
#set -x
BUILD_SCRIPT_DIR=$(realpath $(dirname "${BASH_SOURCE[0]}"))

# Natron's own builds of packages that MSYS2 doesn't provide in the form Natron
# needs (a pinned version, Natron patches, or a licence variant). Everything
# else comes from MSYS2 itself (see the natron-build-deps-* meta-packages).
# Built for the environment of the calling shell (UCRT64).
PKGS="
mingw-w64-natron-setup
mingw-w64-libraw-gpl2
mingw-w64-libraw-lgpl
mingw-w64-openimageio
mingw-w64-seexpr-git
mingw-w64-ffmpeg-gpl2
mingw-w64-imagemagick
mingw-w64-osmesa
mingw-w64-natron-build-deps-common
mingw-w64-natron-build-deps-qt5
mingw-w64-natron-build-deps-qt6
"

source "${BUILD_SCRIPT_DIR}/natron_repo_common.sh"

trap natron_repo_cleanup EXIT

NATRON_REPO_DIR=
if [[ -z $1 ]]; then
  echo "Usage: $(basename $0) <natron_repo_directory>"
  exit 1
else
  NATRON_REPO_DIR=$(realpath $1)
fi

if [[ -z "${MSYSTEM:-}" || "${MSYSTEM}" == MSYS || -z "${MINGW_PACKAGE_PREFIX:-}" ]]; then
  echo "Run this script from an MSYS2 UCRT64 shell (MSYSTEM=UCRT64)."
  exit 1
fi
# makepkg-mingw builds for the architectures in MINGW_ARCH: build for the
# environment of this shell only.
MINGW_ARCH="${MSYSTEM,,}"
echo "Building the Natron packages for ${MSYSTEM} (${MINGW_PACKAGE_PREFIX})."

natron_repo_init ${NATRON_REPO_DIR}

for pkg_dir in ${PKGS}; do
  echo -e "\n\nEntering package directory ${pkg_dir}."
  cd "${BUILD_SCRIPT_DIR}/${pkg_dir}"

  PACKAGE_NAME=`makepkg --printsrcinfo | awk '/pkgname/{print $3}'`

  # Check to see if the package is already available
  if [[ -z $(pacman -Ssq ${PACKAGE_NAME}) ]]; then
    echo "Building ${PACKAGE_NAME} ..."

    # Print available disk space to facilitate GitHub action debugging.
    df -h

    # Remove any package files from previous builds.
    rm -f "${MINGW_PACKAGE_PREFIX}"-*-any.pkg.tar.*

    # Download and verify the sources first, and retry that once: the
    # ffmpeg.org download has timed out several times. makepkg resumes the
    # partial download. Only the download is retried; a build failure below
    # still fails immediately.
    if ! MINGW_ARCH="${MINGW_ARCH}" makepkg-mingw --verifysource --noconfirm; then
      echo "Downloading the sources of ${PACKAGE_NAME} failed, retrying once ..."
      sleep 30
      MINGW_ARCH="${MINGW_ARCH}" makepkg-mingw --verifysource --noconfirm
    fi

    time MINGW_ARCH="${MINGW_ARCH}" MAKEFLAGS="-j$(nproc)" makepkg-mingw -LfCcsr --needed --noconfirm

    echo "Build complete."

    for pkg_file in `find . -name "${MINGW_PACKAGE_PREFIX}-*-any.pkg.tar.*"`; do
      echo "Adding ${pkg_file} to package repo ..."
      natron_repo_add_package ${pkg_file}
    done
  else
    echo "Skipping. ${PACKAGE_NAME} already in repository."
  fi

  echo -e "Leaving package directory ${pkg_dir}."
  cd "${BUILD_SCRIPT_DIR}"
done
