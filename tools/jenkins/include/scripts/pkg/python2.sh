#!/bin/bash

# Install Python2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python2.html
PY2_VERSION=2.7.18
PY2_VERSION_SHORT=${PY2_VERSION%.*}
PY2_TAR="Python-${PY2_VERSION}.tar.xz"
PY2_SITE="https://www.python.org/ftp/python/${PY2_VERSION}"
if download_step; then
    download "$PY2_SITE" "$PY2_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/python2.pc" ] || [ "$(pkg-config --modversion python2)" != "$PY2_VERSION_SHORT" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PY2_TAR"
    pushd "Python-${PY2_VERSION}"
    
    ##########################################################
    ## Patches from https://rpmfind.net/linux/RPM/fedora/devel/rawhide/aarch64/p/python2.7-2.7.18-11.fc35.aarch64.html

    # 00351 # 1ae2a3db6d7af4ea973d1aee285e5fb9f882fdd0
    # Avoid infinite loop when reading specially crafted TAR files using the tarfile module
    # (CVE-2019-20907).
    # See: https://bugs.python.org/issue39017
    git apply -p1 "$INC_PATH"/patches/python27/00351-cve-2019-20907-fix-infinite-loop-in-tarfile.patch

    # 00354 # 73d8baef9f57f26ba97232d116f7220d1801452c
    # Reject control chars in HTTP method in httplib.putrequest to prevent
    # HTTP header injection
    #
    # Backported from Python 3.5-3.10 (and adjusted for py2's single-module httplib):
    # - https://bugs.python.org/issue39603
    # - https://github.com/python/cpython/pull/18485 (3.10)
    # - https://github.com/python/cpython/pull/21946 (3.5)
    patch -Np1 -i "$INC_PATH"/patches/python27/00354-cve-2020-26116-http-request-method-crlf-injection-in-httplib.patch

    # 00355 # fde656e7116767a761720970ee750ecda6774e71
    # No longer call eval() on content received via HTTP in the CJK codec tests
    # Backported from the python3 branches upstream: https://bugs.python.org/issue41944
    # Resolves: https://bugzilla.redhat.com/show_bug.cgi?id=1889886
    patch -Np1 -i "$INC_PATH"/patches/python27/00355-CVE-2020-27619.patch

    # 00357 # c4b8cabe4e772e4b8eea3e4dab5de12a3e9b5bc2
    # CVE-2021-3177: Replace snprintf with Python unicode formatting in ctypes param reprs
    #
    # Backport of Python3 commit 916610ef90a0d0761f08747f7b0905541f0977c7:
    # https://bugs.python.org/issue42938
    # https://github.com/python/cpython/pull/24239
    patch -Np1 -i "$INC_PATH"/patches/python27/00357-CVE-2021-3177.patch

    # 00359 # 0d63bea395d9ba5e281dfbddddd6843cdfc609e5
    # CVE-2021-23336: Add `separator` argument to parse_qs; warn with default
    #
    # Partially backports https://bugs.python.org/issue42967 : [security] Address a web cache-poisoning issue reported in urllib.parse.parse_qsl().
    #
    # Backported from the python3 branch.
    # However, this solution is different than the upstream solution in Python 3.
    #
    # Based on the downstream solution for python 3.6.13 by Petr Viktorin.
    #
    # An optional argument seperator is added to specify the separator.
    # It is recommended to set it to '&' or ';' to match the application or proxy in use.
    # The default can be set with an env variable of a config file.
    # If neither the argument, env var or config file specifies a separator, "&" is used
    # but a warning is raised if parse_qs is used on input that contains ';'.
    patch -Np1 -i "$INC_PATH"/patches/python27/00359-CVE-2021-23336.patch

    # End of Fedora patches
    ##################################################################
    
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --with-system-expat --with-system-ffi --with-ensurepip=yes --enable-unicode=ucs4
    make -j${MKJOBS}
    make install
    chmod -v 755 "$SDK_HOME/lib/libpython2.7.so.1.0"
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --upgrade pip
    popd
    rm -rf "Python-${PY2_VERSION}"
    end_build
fi
