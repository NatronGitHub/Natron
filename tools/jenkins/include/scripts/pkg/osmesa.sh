#!/bin/bash

# Install llvm+osmesa
# Mesa 25.0.7 is the last Mesa release with OSMesa (removed in 25.1): osmesa-install builds it with
# meson (mangled, static, llvmpipe and softpipe drivers), after LLVM 19.1.7, the LLVM of Mesa 25.0's CI.
# Be careful if upgrading: osmesa-install applies important patches.
OSMESA_VERSION=25.0.7
OSMESA_TAR="mesa-${OSMESA_VERSION}.tar.xz"
OSMESA_SHA256=592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6
OSMESA_SITE="https://archive.mesa3d.org"
OSMESA_GLU_VERSION=9.0.1 # 9.0.2 doesn't have glu_mangle.h
OSMESA_GLU_TAR="glu-${OSMESA_GLU_VERSION}.tar.gz"
OSMESA_GLU_SHA256=f6f484cfcd51e489afe88031afdea1e173aa652697e4c19ddbcb8260579a10f7
OSMESA_GLU_SITE="https://archive.mesa3d.org/glu"
OSMESA_LLVM_VERSION=19.1.7
# (LLVM 15 and later are built from the llvm-project tarball)
OSMESA_LLVM_TAR="llvm-project-${OSMESA_LLVM_VERSION}.src.tar.xz"
OSMESA_LLVM_SHA256=82401fea7b79d0078043f7598b835284d6650a75b93e64b6f761ea7b63097501
OSMESA_LLVM_SITE="https://github.com/llvm/llvm-project/releases/download/llvmorg-${OSMESA_LLVM_VERSION}"
# osmesa-install.sh, the script that builds LLVM and Mesa: a fixed commit, as the tarballs
OSMESA_INSTALL_GIT=https://github.com/devernay/osmesa-install
OSMESA_INSTALL_COMMIT=b9817c24ff9f2eee3b55d95e4b6d7fc1a85eda20
# Mesa's meson build needs Python 3 with mako and PyYAML (and packaging, installed by meson.sh).
# sha256 of the PyPI wheels, checked by pip --require-hashes (MarkupSafe and PyYAML: the CPython 3.11
# manylinux wheels).
OSMESA_MAKO_VERSION=1.4.3
OSMESA_MAKO_WHEEL_SHA256=723296007c870bfd6b3f0c3230dba7198096e5269297ebf5e4eff9e7ffa39d4f
OSMESA_MARKUPSAFE_VERSION=3.0.4 # (required by mako)
OSMESA_MARKUPSAFE_WHEEL_SHA256_X86_64=6da83a088f8ef93b2d483a8232a4dbf4d69d3d8496b568a03c56becac43e1808
OSMESA_MARKUPSAFE_WHEEL_SHA256_AARCH64=849dd2bb0e5e4ab2b71c7191726a4a8d5aa8a610daa584728cbee0b710ddc4ef
OSMESA_PYYAML_VERSION=6.0.4
OSMESA_PYYAML_WHEEL_SHA256_X86_64=622aa875fa6533f25fedcffe23079357d65a690d462200afed5d8e9d0e01ebee
OSMESA_PYYAML_WHEEL_SHA256_AARCH64=71bb5b87465d88d1dfc577115af6d435bff9b26f2512936f610c26aa55966028
if download_step; then
    download "$OSMESA_SITE" "$OSMESA_TAR" "$OSMESA_SHA256"
    download "$OSMESA_GLU_SITE" "$OSMESA_GLU_TAR" "$OSMESA_GLU_SHA256"
    download "$OSMESA_LLVM_SITE" "$OSMESA_LLVM_TAR" "$OSMESA_LLVM_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_OSMESA:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/osmesa" || true
    rm -rf "$SDK_HOME/llvm" || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/osmesa/lib/pkgconfig/osmesa.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/osmesa/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion osmesa)" != "$OSMESA_VERSION" ] || [ "$("$SDK_HOME/llvm/bin/llvm-config" --version 2>/dev/null)" != "$OSMESA_LLVM_VERSION" ]; }; }; then
    start_build
    # (no files left from another Mesa version, e.g. Mesa 18's libGL and gl.pc)
    rm -rf "${SDK_HOME}/osmesa"
    mkdir -p "${SDK_HOME}/osmesa"
    LLVM_BUILD=0
    if [ ! -s "$SDK_HOME/llvm/bin/llvm-config" ] || [ "$($SDK_HOME/llvm/bin/llvm-config --version)" != "$OSMESA_LLVM_VERSION" ]; then
        LLVM_BUILD=1
        # (nor from another LLVM version)
        rm -rf "${SDK_HOME}/llvm"
        mkdir -p "${SDK_HOME}/llvm"
    fi
    printf 'mako==%s --hash=sha256:%s\nmarkupsafe==%s --hash=sha256:%s --hash=sha256:%s\npyyaml==%s --hash=sha256:%s --hash=sha256:%s\n' \
        "$OSMESA_MAKO_VERSION" "$OSMESA_MAKO_WHEEL_SHA256" \
        "$OSMESA_MARKUPSAFE_VERSION" "$OSMESA_MARKUPSAFE_WHEEL_SHA256_X86_64" "$OSMESA_MARKUPSAFE_WHEEL_SHA256_AARCH64" \
        "$OSMESA_PYYAML_VERSION" "$OSMESA_PYYAML_WHEEL_SHA256_X86_64" "$OSMESA_PYYAML_WHEEL_SHA256_AARCH64" > osmesa-requirements.txt
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --require-hashes --only-binary :all: -r osmesa-requirements.txt
    rm osmesa-requirements.txt
    git_clone_commit "$OSMESA_INSTALL_GIT" "$OSMESA_INSTALL_COMMIT"
    pushd osmesa-install
    mkdir build
    pushd build
    cp "$SRC_PATH/$OSMESA_TAR" .
    cp "$SRC_PATH/$OSMESA_GLU_TAR" .
    if [ "$LLVM_BUILD" = 1 ]; then
       cp "$SRC_PATH/$OSMESA_LLVM_TAR" .
    fi
    # meson and ninja are the SDK's (meson.sh, ninja.sh). -O3: osmesa-install's default, which CFLAGS
    # would otherwise replace (for GLU; Mesa and LLVM use their release build types).
    env BUILD_OSDEMO=0 CFLAGS="-O3 -fPIC ${GCC14_CFLAGS:-}" CXXFLAGS="-O3 -fPIC" OSMESA_DRIVER=3 OSMESA_PREFIX="${SDK_HOME}/osmesa" OSMESA_VERSION="$OSMESA_VERSION" LLVM_PREFIX="${SDK_HOME}/llvm" LLVM_VERSION="$OSMESA_LLVM_VERSION" MKJOBS="${MKJOBS}" LLVM_BUILD="$LLVM_BUILD" PYTHON="${SDK_HOME}/bin/python${PY3_VERSION_SHORT}" ../osmesa-install.sh
    popd
    popd
    rm -rf osmesa-install
    # Mesa links libdrm and lm-sensors when it finds them (pkg-config, system library directories):
    # the static library would then need system libraries that the bundle doesn't have.
    if nm -u "$SDK_HOME/osmesa/lib/libMangledOSMesa32.a" | grep -qE ' U (drm|sensors_)'; then
        echo "Error: $SDK_HOME/osmesa/lib/libMangledOSMesa32.a uses libdrm or lm-sensors"
        exit 1
    fi
    end_build
fi
