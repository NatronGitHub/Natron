#!/bin/bash

# Install shiboken
# See https://github.com/pyside/pyside-setup
PYSIDE_SETUP_VERSION=5.15.2
PYSIDE_SETUP_GIT="git@github.com:pyside/pyside-setup.git"

if download_step; then
    git_clone_branch_or_tag "${PYSIDE_SETUP_GIT}" "${PYSIDE_SETUP_VERSION}"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/shiboken2.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/lib/pkgconfig:$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion shiboken2)" != "$SHIBOKEN_VERSION" ]; }; }; then
    start_build
    pushd "pyside-setup"

    python setup.py build

    popd
    rm -rf "pyside-setup"
    end_build
fi
