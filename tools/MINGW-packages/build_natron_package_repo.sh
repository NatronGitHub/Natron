#!/bin/sh
set -e
#set -x
BUILD_SCRIPT_DIR=$(realpath $(dirname "${BASH_SOURCE[0]}"))

PKGS="
mingw-w64-natron-setup
mingw-w64-imath
mingw-w64-openexr
mingw-w64-libraw-gpl2
mingw-w64-opencolorio2-git
mingw-w64-openimageio
mingw-w64-seexpr-git
mingw-w64-x264-git
mingw-w64-ffmpeg-gpl2
mingw-w64-sox
mingw-w64-poppler
mingw-w64-imagemagick
mingw-w64-osmesa
mingw-w64-dump_syms
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

natron_repo_init ${NATRON_REPO_DIR}

for pkg_dir in ${PKGS}; do
  echo -e "\n\nEntering package directory ${pkg_dir}."
  cd "${BUILD_SCRIPT_DIR}/${pkg_dir}"

  PACKAGE_NAME=`makepkg --printsrcinfo | awk '/pkgname/{print $3}'`

  # Check to see if the package is already available
  if [[ -z $(pacman -Ssq ${PACKAGE_NAME}) ]]; then
    if [[ ${PACKAGE_NAME} == 'mingw-w64-x86_64-osmesa' ]]; then
      echo "Fetching prebuilt ${PACKAGE_NAME} ..."
      # Can't build osmesa right now so use previously built package.
      OSMESA_PKG_FILENAME=mingw-w64-x86_64-osmesa-111.471db69.17.1.10.3.9.1-2-any.pkg.tar.xz
      wget -O ${OSMESA_PKG_FILENAME} https://sourceforge.net/projects/natron/files/MINGW-packages/mingw64/${OSMESA_PKG_FILENAME}/download
      echo "Fetch complete."
    else
      echo "Building ${PACKAGE_NAME} ..."

      # Print available disk space to facilitate GitHub action debugging.
      df -h

      # Remove any package files from previous builds.
      rm -f mingw-w64-x86_64-*-any.pkg.tar.*

      # Some upstream source hosts are unreliable; ffmpeg.org in particular has
      # aborted this build repeatedly with connection timeouts. A second attempt
      # normally succeeds, so do not lose an hour of work to one bad download.
      build_attempts=2
      for attempt in $(seq 1 ${build_attempts}); do
        if time MAKEFLAGS="-j$(nproc)" makepkg-mingw -LfCcsr --needed --noconfirm; then
          break
        fi
        if [[ ${attempt} -eq ${build_attempts} ]]; then
          echo "Building ${PACKAGE_NAME} failed after ${build_attempts} attempts."
          exit 1
        fi
        echo "Building ${PACKAGE_NAME} failed, retrying (${attempt}/${build_attempts})..."
        sleep 15
      done

      echo "Build complete."
    fi

    for pkg_file in `find . -name 'mingw-w64-x86_64-*-any.pkg.tar.*'`; do
      echo "Adding ${pkg_file} to package repo ..."
      natron_repo_add_package ${pkg_file}
    done
  else
    echo "Skipping. ${PACKAGE_NAME} already in repository."
  fi

  echo -e "Leaving package directory ${pkg_dir}."
  cd "${BUILD_SCRIPT_DIR}"
done
