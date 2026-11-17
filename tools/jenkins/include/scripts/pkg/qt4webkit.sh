#!/bin/bash

# Qt4 webkit (a version more recent than the one in Qt 4.8.7)
# see https://download.kde.org/stable/qtwebkit-2.3/
# see qt-installer.sh
if download_step; then
    download "$QT4WEBKIT_SITE" "$QT4WEBKIT_TAR"
fi
if build_step && { force_build || { [ ! -s "$QT4PREFIX/lib/pkgconfig/QtWebKit.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtWebKit)" != "$QT4WEBKIT_VERSION_PKG" ]; }; }; then
    start_build
    if [ -s /etc/redhat-release ] && [ -x /usr/bin/yum ]; then
        # Most scripts in webkit point to /usr/bin/perl, which is outdated in centos,
        # and doesn't include some of the default packages.
        # We could fix all the scripts to use "/usr/bin/env perl" and "/usr/bin/env swig"
        # instead of "/usr/bin/perl" and "/usr/bin/swig", but for now let's just install
        # the required packages.
        LD_LIBRARY_PATH= PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin yum install perl-Digest-MD5 perl-version -y || true
    fi
    # Check that the system perl has the required modules installed
    /usr/bin/perl -Mversion -e 1
    /usr/bin/perl -MDigest::MD5 -e 1

    # install a more recent qtwebkit, see http://www.linuxfromscratch.org/blfs/view/7.9/x/qt4.html
    mkdir "qtwebkit-${QT4WEBKIT_VERSION}"
    pushd "qtwebkit-${QT4WEBKIT_VERSION}"
    untar "$SRC_PATH/$QT4WEBKIT_TAR"

    #############################################
    # patches from fedora's qtwebkit-2.3.4-35.fc36.src.rpm
    
    # search /usr/lib{,64}/mozilla/plugins-wrapped for browser plugins too
    Patch1=webkit-qtwebkit-2.2-tp1-pluginpath.patch

    # smaller debuginfo s/-g/-g1/ (debian uses -gstabs) to avoid 4gb size limit
    Patch3=qtwebkit-2.3-debuginfo.patch

    # tweak linker flags to minimize memory usage on "small" platforms
    Patch4=qtwebkit-2.3-save_memory.patch

    # use SYSTEM_MALLOC on ppc/ppc64, plus some additional minor tweaks (needed only on ppc? -- rex)
    Patch10=qtwebkit-ppc.patch

    # add missing function Double2Ints(), backport
    # rebased for 2.3.1, not sure if this is still needed?  -- rex
    Patch11=qtwebkit-23-LLInt-C-Loop-backend-ppc.patch

    # truly madly deeply no rpath please, kthxbye
    Patch14=webkit-qtwebkit-23-no_rpath.patch

    ## upstream patches
    # backport from qt5-qtwebkit
    # qtwebkit: undefined symbol: g_type_class_adjust_private_offset
    # https://bugzilla.redhat.com/show_bug.cgi?id=1202735
    Patch100=webkit-qtwebkit-23-gcc5.patch
    # backport from qt5-qtwebkit: URLs visited during private browsing show up in WebpageIcons.db
    Patch101=webkit-qtwebkit-23-private_browsing.patch
    # fix FTBFS with bison-3.7
    Patch102=qtwebkit-bison-3.7.patch
    # fix FTBFS wtih glib ≥ 2.68
    Patch103=webkit-qtwebkit-23-glib2.patch

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch1" # -p1 -b .pluginpath
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch3" # -p1 -b .debuginfo
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch4" # -p1 -b .save_memory
    #%ifarch ppc ppc64 ppc64le s390 s390x %{mips}
    #patch -Np1 -i "$INC_PATH/patches/Qt/$Patch10" # -p1 -b .system-malloc
    #%endif
    #%ifarch ppc ppc64 s390 s390x mips mips64
    ## all big-endian arches require the Double2Ints fix
    ## still needed?  -- rex
    #patch -Np1 -i "$INC_PATH/patches/Qt/$Patch11" # -p1 -b .Double2Ints
    #%endif
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch14" # -p1 -b .no_rpath

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch100" # -p1 -b .gcc5
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch101" # -p1 -b .private_browsing
    #%if 0%{?fedora} > 33 || 0%{?rhel} > 8 # bison ≥ 3.7
    if version_gt "$BISON_VERSION" 3.6.99; then
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch102" # -p1 -b .bison37
    fi
    #%if 0%{?fedora} > 34 || 0%{?rhel} > 8 # glib ≥ 2.68
    if version_gt "$GLIB_VERSION" 2.67.99; then
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch103" #-p1 -b .glib2
    fi

    #######################################
    
    # disable xslt if libxml2 is compiled with ICU support, or qtwebkit will not compile, see https://aur.archlinux.org/packages/qtwebkit
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        xslt_flag="--no-xslt"
    else
        xslt_flag=""
    fi
    QTDIR="$QT4PREFIX" PATH="$QT4PREFIX/bin:$PATH" \
         Tools/Scripts/build-webkit --qt            \
         --no-webkit2    \
         "$xslt_flag" \
         --prefix="$QT4PREFIX" --makeargs=-j${MKJOBS}
    make -C WebKitBuild/Release install -j${MKJOBS}
    popd
    rm -rf "qtwebkit-${QT4WEBKIT_VERSION}"
    end_build
fi

