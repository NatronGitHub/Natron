#!/bin/bash

# Install qtifw
QTIFW_GIT=https://github.com/NatronGitHub/installer-framework.git
if build_step && { force_build || { [ ! -s $SDK_HOME/installer/bin/binarycreator ]; }; }; then
    start_build
    git_clone_branch_or_tag "$QTIFW_GIT" 1.6-natron
    pushd installer-framework
    "$SDK_HOME/installer/bin/qmake"
    make -j${MKJOBS}
    strip -s bin/*
    cp bin/* "$SDK_HOME/installer/bin/"
    popd
    rm -rf installer-framework
    end_build
fi
