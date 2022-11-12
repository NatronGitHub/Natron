#!/bin/bash

# Install Python2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python2.html
PY2_VERSION=2.7.18
PY2_VERSION_SHORT=${PY2_VERSION%.*}
PY2_TAR="Python-${PY2_VERSION}.tar.xz"
PY2_SITE="https://www.python.org/ftp/python/${PY2_VERSION}"

PY2VER="${PY2_VERSION_SHORT}"
PY2VERNODOT=$(echo ${PY2VER:-}| sed 's/\.//')
PY2_EXE=$SDK_HOME/bin/python2
PY2_LIB=$SDK_HOME/lib/libpython${PY2VER}.so
PY2_INC=$SDK_HOME/include/python${PY2VER}
if [ -z "${PYV:-}" ] || [ "$PYV" = "2" ]; then
    PYV=2
    PYVER="${PY2VER}"
    PYVERNODOT="${PY2VERNODOT}"
    PY_EXE="$SDK_HOME/bin/python${PYV}"
    PY_LIB="$SDK_HOME/lib/libpython${PYVER}.so"
    PY_INC="$SDK_HOME/include/python${PYVER}"
    PYTHON_HOME="$SDK_HOME"
    PYTHON_PATH="$SDK_HOME/lib/python${PYVER}"
    PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
    export PYTHON_PATH PYTHON_INCLUDE
fi

if download_step; then
    download "$PY2_SITE" "$PY2_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/python2.pc" ] || [ "$(pkg-config --modversion python2)" != "$PY2_VERSION_SHORT" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PY2_TAR"
    pushd "Python-${PY2_VERSION}"
    
    ##########################################################
    ## Patches from python2.7-2.7.18-27.fc38 https://src.fedoraproject.org/rpms/python2.7

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

    # 00361 # b86ebbab187c809f536620e2cbf1f370886979a9
    # Make python2.7 compatible with OpenSSL 3.0.0
    #
    # Backported from python3.
    #
    # Based on https://github.com/tiran/cpython/tree/2.7.18-openssl3
    if version_gt "$OPENSSL_VERSION" 1.99.9999; then
        # breaks the build with openssl 1.1
        patch -Np1 -i "$INC_PATH"/patches/python27/00361-openssl-3-compat.patch
    fi

    # 00366 # e76b05ea3313854adf80e290c07d5b38fef606bb
    # CVE-2021-3733: Fix ReDoS in urllib AbstractBasicAuthHandler
    #
    # Fix Regular Expression Denial of Service (ReDoS) vulnerability in
    # urllib2.AbstractBasicAuthHandler. The ReDoS-vulnerable regex
    # has quadratic worst-case complexity and it allows cause a denial of
    # service when identifying crafted invalid RFCs. This ReDoS issue is on
    # the client side and needs remote attackers to control the HTTP server.
    #
    # Backported from Python 3 together with another backward-compatible
    # improvement of the regex from fix for CVE-2020-8492.
    patch -Np1 -i "$INC_PATH"/patches/python27/00366-CVE-2021-3733.patch

    # 00368 # 10dcf6732fb101ce89ad506a89365c6b1ff8c4e4
    # CVE-2021-3737: http client infinite line reading (DoS) after a HTTP 100 Continue
    #
    # Fixes http.client potential denial of service where it could get stuck reading
    # lines from a malicious server after a 100 Continue response.
    #
    # Backported from Python 3.
    patch -Np1 -i "$INC_PATH"/patches/python27/00368-CVE-2021-3737.patch

    # 00372 # 0039a5762ac21255d6e4c2421a2159cbecfc95bd
    # CVE-2021-4189: ftplib should not use the host from the PASV response
    #
    # Upstream: https://bugs.python.org/issue43285
    #
    # Backported from the python3 branch.
    patch -Np1 -i "$INC_PATH"/patches/python27/00372-CVE-2021-4189.patch

    # 00375 # 5488ab84d2447aa8df8b3502e76f151ac2488947
    # Fix precision in test_distance (test.test_turtle.TestVec2D).
    # See: https://bugzilla.redhat.com/show_bug.cgi?id=2038843
    patch -Np1 -i "$INC_PATH"/patches/python27/00375-fix-test_distance-to-enable-Python-build-on-i686.patch

    # 00377 # 53940f806f5e3451c9b77b44e2fc4d18b243156b
    # CVE-2022-0391: urlparse does not sanitize URLs containing ASCII newline and tabs
    #
    # ASCII newline and tab characters are stripped from the URL.
    #
    # Upstream: https://bugs.python.org/issue43882
    #
    # Backported from Python 3.
    patch -Np1 -i "$INC_PATH"/patches/python27/00377-CVE-2022-0391.patch

    # 00378 # 32b7f6200303fb36d104ac8d2afa3afba1789c1b
    # Support expat 2.4.5
    #
    # Curly brackets were never allowed in namespace URIs
    # according to RFC 3986, and so-called namespace-validating
    # XML parsers have the right to reject them a invalid URIs.
    #
    # libexpat >=2.4.5 has become strcter in that regard due to
    # related security issues; with ET.XML instantiating a
    # namespace-aware parser under the hood, this test has no
    # future in CPython.
    #
    # References:
    # - https://datatracker.ietf.org/doc/html/rfc3968
    # - https://www.w3.org/TR/xml-names/
    #
    # Also, test_minidom.py: Support Expat >=2.4.5
    #
    # Upstream: https://bugs.python.org/issue46811
    #
    # Backported from Python 3.
    patch -Np1 -i "$INC_PATH"/patches/python27/00378-support-expat-2-4-5.patch

    # 00382 # 65796028be4b28aaf027963d6fb2ef4c219e73dc
    # Make mailcap refuse to match unsafe filenames/types/params (GH-91993)
    #
    # Upstream: https://github.com/python/cpython/issues/68966
    #
    # Tracker bug: https://bugzilla.redhat.com/show_bug.cgi?id=2075390
    #
    # Backported from python3.
    patch -Np1 -i "$INC_PATH"/patches/python27/00382-cve-2015-20107.patch

    # (New patches go here ^^^)
    # End of Fedora patches
    ##################################################################
    
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --with-system-expat --with-system-ffi --with-ensurepip=yes --enable-unicode=ucs4
    make -j${MKJOBS}
    make install
    chmod -v 755 "$SDK_HOME/lib/libpython2.7.so.1.0"
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --upgrade pip setuptools wheel
    popd
    rm -rf "Python-${PY2_VERSION}"
    end_build
fi
