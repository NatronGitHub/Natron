#!/bin/bash
set -e
#set -x
BUILD_SCRIPT_DIR=$(realpath $(dirname "${BASH_SOURCE[0]}"))

# Natron's own builds of packages that MSYS2 doesn't provide in the form Natron
# needs (a pinned version, Natron patches, or a licence variant). Everything
# else comes from MSYS2 itself (see the natron-build-deps-* meta-packages).
# Built for the environment of the calling shell (UCRT64 or CLANGARM64); a
# package whose mingw_arch doesn't list it is skipped.
# mingw-w64-freetype and mingw-w64-qt5-base (CLANGARM64 only) replace MSYS2's
# packages (NATRON_REPO_OVERRIDES in natron_repo_common.sh), and are built
# first so that the packages that follow are built and checked with them.
PKGS="
mingw-w64-freetype
mingw-w64-qt5-base
mingw-w64-natron-setup
mingw-w64-libraw-gpl2
mingw-w64-libraw-lgpl
mingw-w64-openimageio
mingw-w64-seexpr-git
mingw-w64-ffmpeg-gpl2
mingw-w64-ffmpeg-lgpl
mingw-w64-libheif-lgpl
mingw-w64-libtiff-lgpl
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

# The packages are built for UCRT64 (x86-64, gcc) and CLANGARM64 (ARM64, clang,
# #1494), as listed in the mingw_arch of each PKGBUILD: MSYS2's MINGW64 lacks
# several of their dependencies.
case "${MSYSTEM:-}" in
  UCRT64|CLANGARM64) ;;
  *)
    echo "Run this script from an MSYS2 UCRT64 or CLANGARM64 shell (current: MSYSTEM=${MSYSTEM:-unset})."
    exit 1
    ;;
esac
if [[ -z "${MINGW_PACKAGE_PREFIX:-}" ]]; then
  echo "MINGW_PACKAGE_PREFIX is not set: run this script from an MSYS2 ${MSYSTEM} login shell."
  exit 1
fi
# makepkg-mingw builds for the architectures in MINGW_ARCH.
MINGW_ARCH="${MSYSTEM,,}"
echo "Building the Natron packages for ${MSYSTEM} (${MINGW_PACKAGE_PREFIX})."

# pkg_builds_here <package directory>: whether the mingw_arch of its PKGBUILD lists MINGW_ARCH
pkg_builds_here() {
  (
    # mingw_arch is a plain array assignment in every PKGBUILD: read only that line
    mingw_arch=()
    eval "$(grep -E '^mingw_arch=' "$1/PKGBUILD")"
    for a in "${mingw_arch[@]}"; do
      [[ "$a" == "${MINGW_ARCH}" ]] && exit 0
    done
    exit 1
  )
}

natron_repo_init ${NATRON_REPO_DIR}

for pkg_dir in ${PKGS}; do
  if ! pkg_builds_here "${BUILD_SCRIPT_DIR}/${pkg_dir}"; then
    echo -e "\n\nSkipping ${pkg_dir}: not built for ${MINGW_ARCH} (mingw_arch in its PKGBUILD)."
    continue
  fi
  echo -e "\n\nEntering package directory ${pkg_dir}."
  cd "${BUILD_SCRIPT_DIR}/${pkg_dir}"

  # the first pkgname: a split package (qt5-base) also builds qt5-base-debug, which is not added
  PACKAGE_NAME=`makepkg --printsrcinfo | awk '$1 == "pkgname" {print $3; exit}'`

  # Check to see if the package is already in the natron repository. Only look
  # there: a package that replaces an MSYS2 package (mingw-w64-freetype) has
  # the same name as one in the MSYS2 repositories.
  if ! pacman -Sl natron 2>/dev/null | awk '{print $2}' | grep -qxF "${PACKAGE_NAME}"; then
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
      # not the separate debug info of a split package (qt5-base-debug: hundreds of MB, which
      # neither the builds nor the installers use)
      if [[ $(basename "${pkg_file}") == "${PACKAGE_NAME}"-debug-[0-9]* ]]; then
        echo "Not adding ${pkg_file} (debug info) to package repo."
        continue
      fi
      echo "Adding ${pkg_file} to package repo ..."
      natron_repo_add_package ${pkg_file}
    done
  else
    echo "Skipping. ${PACKAGE_NAME} already in repository."
  fi

  # Install a package that replaces MSYS2's (NATRON_REPO_OVERRIDES) as soon as
  # it is in the repository, whether just built or restored from the cache,
  # without --overwrite: this also checks that it installs cleanly over MSYS2's.
  for name in ${NATRON_REPO_OVERRIDES}; do
    if [[ ${PACKAGE_NAME} == "${MINGW_PACKAGE_PREFIX}-${name}" ]]; then
      natron_repo_install_overrides
      break
    fi
  done

  echo -e "Leaving package directory ${pkg_dir}."
  cd "${BUILD_SCRIPT_DIR}"
done
