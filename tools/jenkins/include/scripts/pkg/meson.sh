#!/bin/bash

# Install meson
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/meson.html
MESON_VERSION=0.53.1
if build_step && { force_build || { [ ! -x "$SDK_HOME/bin/meson" ] || [ $("$SDK_HOME/bin/meson" --version) != ${MESON_VERSION} ];  }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary meson meson=="${MESON_VERSION}"
    end_build
fi
