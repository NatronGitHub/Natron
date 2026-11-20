#!/bin/bash

# Install openssl
# see openssl-installer.sh
# These are the default options from Fedora https://src.fedoraproject.org/rpms/openssl:
# zlib enable-camellia enable-seed enable-rfc3779 enable-sctp enable-cms enable-md2 enable-rc5 enable-ssl3 enable-ssl3-method enable-weak-ssl-ciphers no-mdc2 no-ec2m no-sm2 no-sm4"
#
# We need enable-ssl3-method, or legacy code (including Qt4) may fail to compile. See:
# - https://code.qt.io/cgit/qt/qtbase.git/commit?id=6839aead0430a9b07b60fa3a1a7d685fe5d2d1ef
# - https://trac.macports.org/ticket/58205#comment:9
#
# Working config for the SDK openssl:
#OPENSSL_CONFIG="enable-cms enable-camellia enable-idea enable-mdc2 enable-tlsext enable-rfc3779 enable-ssl3-method"
# The version is set by openssl-installer.sh, which is only built with WITH_QT4 (keep them in sync)
# OpenSSL 1.1.1 is end-of-life: use OpenSSL 3 (as MacPorts) unless the Qt 4 installer needs 1.1.1.
OPENSSL_VERSION="${OPENSSL_VERSION:-3.6.5}"
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz"
if version_gt 3.0.0 "$OPENSSL_VERSION"; then
    OPENSSL_SITE="https://www.openssl.org/source"
else
    OPENSSL_SITE="https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}"
fi
OPENSSL_CONFIG="enable-camellia enable-seed enable-rfc3779 enable-cms enable-md2 enable-rc5 enable-ssl3 enable-ssl3-method enable-weak-ssl-ciphers no-mdc2 no-ec2m no-sm2 no-sm4"
if download_step; then
    download "$OPENSSL_SITE" "$OPENSSL_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    # Patch from http://www.linuxfromscratch.org/patches/downloads/openssl/
    if version_gt 1.1.0 "$OPENSSL_VERSION"; then
        patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix="$SDK_HOME" shared ${OPENSSL_CONFIG}
    perl configdata.pm --dump
    # no parallel build for openssl < 1.1.0, and it's safer to avoid parallel build
    # for a crypto library anyway:
    # https://github.com/openssl/openssl/issues/5762
    # https://github.com/openssl/openssl/issues/2331
    # https://github.com/openssl/openssl/issues/298
    make
    make install
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build
fi
