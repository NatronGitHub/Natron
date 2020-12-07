#!/bin/bash
if [ -z "${UBUNTU:-}" ] && [ "${CENTOS:-8}" -le 7 ]; then # breakpad does not compile yet on CentOS 8 or Ubuntu >= 18.04

# Install breakpad tools
BREAKPAD_GIT=https://github.com/NatronGitHub/breakpad.git # breakpad for server-side or linux dump_syms, not for Natron!
BREAKPAD_GIT_COMMIT=f264b48eb0ed8f0893b08f4f9c7ae9685090ccb8
if build_step && { force_build || { [ ! -s "${SDK_HOME}/bin/dump_syms" ]; }; }; then

    start_build
    rm -f breakpad || true
    git_clone_commit "$BREAKPAD_GIT" "$BREAKPAD_GIT_COMMIT"
    pushd breakpad
    git submodule update -i

    # Patch bug due to fix of glibc
    patch -Np1 -i "$INC_PATH"/patches/breakpad/0002-Replace-remaining-references-to-struct-ucontext-with.patch

    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure -prefix "$SDK_HOME"
    make
    cp src/tools/linux/dump_syms/dump_syms "$SDK_HOME/bin/"
    popd
    rm -rf breakpad
    end_build
fi

fi
