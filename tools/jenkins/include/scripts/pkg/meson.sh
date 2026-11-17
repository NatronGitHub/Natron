#!/bin/bash

# Install meson
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/meson.html
MESON_VERSION=1.1.0
#MESON_TAR="meson-${MESON_VERSION}.tar.gz"
#MESON_SITE="https://github.com/mesonbuild/meson/releases/download/v${MESON_VERSION}"

# Installed via pip
#if download_step; then
#    download "$MESON_SITE" "$MESON_TAR"
#fi
if build_step && { force_build || { [ ! -x "$SDK_HOME/bin/meson" ] || [ $("$SDK_HOME/bin/meson" --version) != ${MESON_VERSION} ];  }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary meson meson=="${MESON_VERSION}"
    end_build
fi
