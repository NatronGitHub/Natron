#!/bin/bash

# Install meson
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/meson.html
MESON_VERSION=1.12.1
# sha256 of the PyPI wheels (pure Python), checked by pip --require-hashes
MESON_WHEEL_SHA256=930bc7542cbd9f57009e182fd014eba48cf1a0180a7b9006d2d32d8f168d8b02
PACKAGING_VERSION=26.3 # glib >= 2.80 needs the packaging module
PACKAGING_WHEEL_SHA256=d7193f7c8e4e93f444fde0262bf90af30e16fa0ad0ad44cb553c87339b23cd1c
#MESON_TAR="meson-${MESON_VERSION}.tar.gz"
#MESON_SITE="https://github.com/mesonbuild/meson/releases/download/v${MESON_VERSION}"

# Installed via pip
#if download_step; then
#    download "$MESON_SITE" "$MESON_TAR"
#fi
if build_step && { force_build || { [ ! -x "$SDK_HOME/bin/meson" ] || [ $("$SDK_HOME/bin/meson" --version) != ${MESON_VERSION} ];  }; }; then
    start_build
    printf 'meson==%s --hash=sha256:%s\npackaging==%s --hash=sha256:%s\n' "$MESON_VERSION" "$MESON_WHEEL_SHA256" "$PACKAGING_VERSION" "$PACKAGING_WHEEL_SHA256" > meson-requirements.txt
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --require-hashes --only-binary :all: -r meson-requirements.txt
    rm meson-requirements.txt
    end_build
fi
