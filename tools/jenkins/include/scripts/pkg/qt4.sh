#!/bin/bash

# Qt4
if build_step && { force_build || { [ "${REBUILD_QT4:-}" = "1" ]; }; }; then
    rm -rf "$QT4PREFIX" || true
fi
if dobuild; then
    export QTDIR="$QT4PREFIX"
fi
if build_step && { force_build || { [ ! -s "$QT4PREFIX/lib/pkgconfig/QtCore.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtCore)" != "$QT4_VERSION" ]; }; }; then
    start_build
    QT_CONF=( "-v" "-system-zlib" "-system-libtiff" "-system-libpng" "-no-libmng" "-system-libjpeg" "-no-gtkstyle" "-glib" "-xrender" "-xrandr" "-xcursor" "-xfixes" "-xinerama" "-fontconfig" "-xinput" "-sm" "-no-multimedia" "-confirm-license" "-release" "-opensource" "-dbus-linked" "-opengl" "desktop" "-nomake" "demos" "-nomake" "docs" "-nomake" "examples" "-optimized-qmake" )
    # qtwebkit is installed separately (see below)
    # also install the sqlite plugin.
    # link with the SDK's openssl
    QT_CONF+=( "-no-webkit" "-plugin-sql-sqlite" "-system-sqlite" "-openssl-linked" )
    # Add MariaDB/MySQL plugin
    QT_CONF+=( "-plugin-sql-mysql" )
    # Add PostgresSQL plugin
    QT_CONF+=( "-plugin-sql-psql" )
    # icu must be enabled in Qt, since it is enabled in libxml2
    # see https://bugs.webkit.org/show_bug.cgi?id=82824#c1
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        QT_CONF+=( "-icu" )
    else
        QT_CONF+=( "-no-icu" )
    fi
    # disable phonon (can be provided by http://www.linuxfromscratch.org/blfs/view/7.9/kde/phonon.html)
    QT_CONF+=( "-no-phonon" "-no-phonon-backend" )
    # disable openvg
    QT_CONF+=( "-no-openvg" )
    
    download "$QT4_SITE" "$QT4_TAR"
    untar "$SRC_PATH/$QT4_TAR"
    pushd "qt-everywhere-opensource-src-${QT4_VERSION}"

    ############################################################
    # Fedora patches

    ## Patches from https://download.fedoraproject.org/pub/fedora/linux/development/rawhide/Everything/source/tree/Packages/q/qt-4.8.7-33.fc28.src.rpm

    # set default QMAKE_CFLAGS_RELEASE
    Patch2=qt-everywhere-opensource-src-4.8.0-tp-multilib-optflags.patch

    # get rid of timestamp which causes multilib problem
    Patch4=qt-everywhere-opensource-src-4.8.5-uic_multilib.patch

    # reduce debuginfo in qtwebkit (webcore)
    Patch5=qt-everywhere-opensource-src-4.8.5-webcore_debuginfo.patch

    # cups16 printer discovery
    Patch6=qt-cupsEnumDests.patch

    # prefer adwaita over gtk+ on DE_GNOME
    # https://bugzilla.redhat.com/show_bug.cgi?id=1192453
    Patch10=qt-prefer_adwaita_on_gnome.patch

    # enable ft lcdfilter
    Patch15=qt-x11-opensource-src-4.5.1-enable_ft_lcdfilter.patch

    # may be upstreamable, not sure yet
    # workaround for gdal/grass crashers wrt glib_eventloop null deref's
    Patch23=qt-everywhere-opensource-src-4.6.3-glib_eventloop_nullcheck.patch

    # hack out largely useless (to users) warnings about qdbusconnection
    # (often in kde apps), keep an eye on https://git.reviewboard.kde.org/r/103699/
    Patch25=qt-everywhere-opensource-src-4.8.3-qdbusconnection_no_debug.patch

    # lrelease-qt4 tries to run qmake not qmake-qt4 (http://bugzilla.redhat.com/820767)
    Patch26=qt-everywhere-opensource-src-4.8.1-linguist_qmake-qt4.patch

    # enable debuginfo in libQt3Support
    Patch27=qt-everywhere-opensource-src-4.8.1-qt3support_debuginfo.patch

    # kde4/multilib QT_PLUGIN_PATH
    Patch28=qt-everywhere-opensource-src-4.8.5-qt_plugin_path.patch

    ## upstreamable bits
    # add support for pkgconfig's Requires.private to qmake
    Patch50=qt-everywhere-opensource-src-4.8.4-qmake_pkgconfig_requires_private.patch

    # FTBFS against newer firebird
    Patch51=qt-everywhere-opensource-src-4.8.7-firebird.patch

    # workaround major/minor macros possibly being defined already
    Patch52=qt-everywhere-opensource-src-4.8.7-QT_VERSION_CHECK.patch

    # fix invalid inline assembly in qatomic_{i386,x86_64}.h (de)ref implementations
    Patch53=qt-x11-opensource-src-4.5.0-fix-qatomic-inline-asm.patch

    # fix invalid assumptions about mysql_config --libs
    # http://bugzilla.redhat.com/440673
    Patch54=qt-everywhere-opensource-src-4.8.5-mysql_config.patch

    # http://bugs.kde.org/show_bug.cgi?id=180051#c22
    Patch55=qt-everywhere-opensource-src-4.6.2-cups.patch

    # backport https://codereview.qt-project.org/#/c/205874/
    Patch56=qt-everywhere-opensource-src-4.8.7-mariadb.patch

    # Fails to create debug build of Qt projects on mingw (rhbz#653674)
    Patch64=qt-everywhere-opensource-src-4.8.5-QTBUG-14467.patch

    # fix QTreeView crash triggered by KPackageKit (patch by David Faure)
    Patch65=qt-everywhere-opensource-src-4.8.0-tp-qtreeview-kpackagekit-crash.patch

    # fix the outdated standalone copy of JavaScriptCore
    Patch67=qt-everywhere-opensource-src-4.8.6-s390.patch

    # https://bugs.webkit.org/show_bug.cgi?id=63941
    # -Wall + -Werror = fail
    Patch68=qt-everywhere-opensource-src-4.8.3-no_Werror.patch

    # revert qlist.h commit that seems to induce crashes in qDeleteAll<QList (QTBUG-22037)
    Patch69=qt-everywhere-opensource-src-4.8.0-QTBUG-22037.patch

    # Buttons in Qt applications not clickable when run under gnome-shell (#742658, QTBUG-21900)
    Patch71=qt-everywhere-opensource-src-4.8.5-QTBUG-21900.patch

    # workaround
    # sql/drivers/tds/qsql_tds.cpp:341:49: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
    Patch74=qt-everywhere-opensource-src-4.8.5-tds_no_strict_aliasing.patch

    # add missing method for QBasicAtomicPointer on s390(x)
    Patch76=qt-everywhere-opensource-src-4.8.0-s390-atomic.patch

    # don't spam in release/no_debug mode if libicu is not present at runtime
    Patch77=qt-everywhere-opensource-src-4.8.3-icu_no_debug.patch

    # https://bugzilla.redhat.com/show_bug.cgi?id=810500
    Patch81=qt-everywhere-opensource-src-4.8.2--assistant-crash.patch

    # https://bugzilla.redhat.com/show_bug.cgi?id=694385
    # https://bugs.kde.org/show_bug.cgi?id=249217
    # https://bugreports.qt-project.org/browse/QTBUG-4862
    # QDir::homePath() should account for an empty HOME environment variable on X11
    Patch82=qt-everywhere-opensource-src-4.8.5-QTBUG-4862.patch

    # poll support
    Patch83=qt-4.8-poll.patch

    # fix QTBUG-35459 (too low entityCharacterLimit=1024 for CVE-2013-4549)
    Patch84=qt-everywhere-opensource-src-4.8.5-QTBUG-35459.patch

    # systemtrayicon plugin support (for appindicators)
    Patch86=qt-everywhere-opensource-src-4.8.6-systemtrayicon.patch

    # fixes for LibreOffice from the upstream Qt bug tracker (#1105422):
    Patch87=qt-everywhere-opensource-src-4.8.6-QTBUG-37380.patch
    Patch88=qt-everywhere-opensource-src-4.8.6-QTBUG-34614.patch
    Patch89=qt-everywhere-opensource-src-4.8.6-QTBUG-38585.patch

    # build against the system clucene09-core
    Patch90=qt-everywhere-opensource-src-4.8.6-system-clucene.patch

    # fix arch autodetection for 64-bit MIPS
    Patch91=qt-everywhere-opensource-src-4.8.7-mips64.patch

    # fix build issue(s) with gcc6
    Patch100=qt-everywhere-opensource-src-4.8.7-gcc6.patch

    # support alsa-1.1.x
    Patch101=qt-everywhere-opensource-src-4.8.7-alsa-1.1.patch

    # upstream patches
    # backported from Qt5 (essentially)
    # http://bugzilla.redhat.com/702493
    # https://bugreports.qt-project.org/browse/QTBUG-5545
    Patch102=qt-everywhere-opensource-src-4.8.5-qgtkstyle_disable_gtk_theme_check.patch
    # workaround for MOC issues with Boost headers (#756395)
    # https://bugreports.qt-project.org/browse/QTBUG-22829
    Patch113=qt-everywhere-opensource-src-4.8.6-QTBUG-22829.patch

    # aarch64 support, https://bugreports.qt-project.org/browse/QTBUG-35442
    Patch180=qt-aarch64.patch

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch4" # -p1 -b .uic_multilib
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch5" # -p1 -b .webcore_debuginfo
    # ie, where cups-1.6+ is present
    #%if 0%{?fedora} || 0%{?rhel} > 7
    ##patch6 -p1 -b .cupsEnumDests
    #%endif
    patch -Np0 -i "$INC_PATH/patches/Qt/$Patch10" # -p0 -b .prefer_adwaita_on_gnome
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch15" # -p1 -b .enable_ft_lcdfilter
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch23" # -p1 -b .glib_eventloop_nullcheck
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch25" # -p1 -b .qdbusconnection_no_debug
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch26" # -p1 -b .linguist_qtmake-qt4
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch27" # -p1 -b .qt3support_debuginfo
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch28" # -p1 -b .qt_plugin_path
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch50" # -p1 -b .qmake_pkgconfig_requires_private
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch51" # -p1 -b .firebird
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch52" # -p1 -b .QT_VERSION_CHECK
    ## TODO: still worth carrying?  if so, upstream it.
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch53" # -p1 -b .qatomic-inline-asm
    ## TODO: upstream me
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch54" # -p1 -b .mysql_config
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch55" # -p1 -b .cups-1
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch56" # -p1 -b .mariadb
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch64" # -p1 -b .QTBUG-14467
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch65" # -p1 -b .qtreeview-kpackagekit-crash
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch67" # -p1 -b .s390
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch68" # -p1 -b .no_Werror
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch69" # -p1 -b .QTBUG-22037
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch71" # -p1 -b .QTBUG-21900
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch74" # -p1 -b .tds_no_strict_aliasing
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch76" # -p1 -b .s390-atomic
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch77" # -p1 -b .icu_no_debug
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch81" # -p1 -b .assistant-crash
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch82" # -p1 -b .QTBUG-4862
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch83" # -p1 -b .poll
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch87" # -p1 -b .QTBUG-37380
    patch -Np0 -i "$INC_PATH/patches/Qt/$Patch88" # -p0 -b .QTBUG-34614
    patch -Np0 -i "$INC_PATH/patches/Qt/$Patch89" # -p0 -b .QTBUG-38585

    #%if 0%{?system_clucene}
    #patch -Np1 -i "$INC_PATH/patches/Qt/$Patch90" # -p1 -b .system_clucene
    ## delete bundled copy
    #rm -rf src/3rdparty/clucene
    #%endif
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch91" # -p1 -b .mips64
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch100" # -p1 -b .gcc6
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch101" # -p1 -b .alsa1.1

    # upstream patches
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch102" # -p1 -b .qgtkstyle_disable_gtk_theme_check
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch113" # -p1 -b .QTBUG-22829

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch180" # -p1 -b .aarch64

    # upstream git

    # security fixes
    # regression fixes for the security fixes
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch84" # -p1 -b .QTBUG-35459

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch86" # -p1 -b .systemtrayicon

    #####################################################################
    # Natron-specific patches
    
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-qt-custom-threadpool.diff
    patch -Np0 -i "$INC_PATH"/patches/Qt/qt4-kde-fix.diff


    #####################################################################
    # MacPorts/homebrew patches

    # (25) avoid zombie processes; see also:
    # https://trac.macports.org/ticket/46608
    # https://codereview.qt-project.org/#/c/61294/
    # approved but abandoned.
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-src_corelib_io_qprocess_unix.cpp.diff    
    # (28) Better invalid fonttable handling
    # Qt commit 0a2f2382 on July 10, 2015 at 7:22:32 AM EDT.
    # not included in 4.8.7 release.
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-better-invalid-fonttable-handling.diff
    # (30) Backport of Qt5 patch to fix an issue with null bytes in QSetting strings (QTBUG-56124).
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-qsettings-null.diff
    # avoid conflict with newer versions of pcre, see eg https://github.com/LLNL/spack/pull/4270
    patch -Np0 -i "$INC_PATH"/patches/Qt/qt4-pcre.patch
    patch -Np1 -i "$INC_PATH"/patches/Qt/0001-Enable-building-with-C-11-and-C-14.patch
    patch -Np1 -i "$INC_PATH"/patches/Qt/qt4-selection-flags-static_cast.patch
    if version_gt "$OPENSSL_VERSION" 1.0.9999; then
        # OpenSSL 1.1 support from ArchLinux https://aur.archlinux.org/cgit/aur.git/tree/qt4-openssl-1.1.patch?h=qt4
        patch -Np1 -i "$INC_PATH"/patches/Qt/qt4-openssl-1.1.patch
    fi

    #QT_SRC="$(pwd)/src"
    env CFLAGS="$BF" CXXFLAGS="$BF" OPENSSL_LIBS="-L$SDK_HOME/lib -lssl -lcrypto" ./configure -prefix "$QT4PREFIX" "${QT_CONF[@]}" -shared

    # https://bugreports.qt-project.org/browse/QTBUG-5385
    LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" make -j${MKJOBS} || exit  1

    make install
    if [ -d "$QT4PREFIX/tests" ]; then
        rm -rf "$QT4PREFIX/tests"
    fi
    #  Remove references to the build directory from the installed .pc files by running the following command as the root user:
    find $QT4PREFIX/lib/pkgconfig -name "*.pc" -exec perl -pi -e "s, -L$PWD/?\S+,,g" {} \;
    #  Remove references to the build directory in the installed library dependency (prl) files by running the following command as the root user:
    for file in $QT4PREFIX/lib/libQt*.prl; do
        sed -r -e '/^QMAKE_PRL_BUILD_DIR/d'    \
            -e 's/(QMAKE_PRL_LIBS =).*/\1/' \
            -i $file
    done
    popd
    rm -rf "qt-everywhere-opensource-src-${QT4_VERSION}"
    end_build
fi
