# Maintainer: Alexey Pavlov <alexpux@gmail.com>

# Please keep this file exactly the same (except _variant=)
# as {mingw-w64-qt4/PKGBUILD,mingw-w64-qt4-static/PKGBUILD}
#_variant=-static
_variant=-shared

if [ "${_variant}" = "-static" ]; then
  _namesuff="-static"
else
  _namesuff=
fi

_realname=qt4${_namesuff}
pkgbase="mingw-w64-${_realname}"
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"

# qt4-static must be kept in its own prefix hierarchy
# as otherwise it will conflict with qt4 itself
if [ "${_variant}" = "-static" ]; then
  _qt4_prefix="${MINGW_PREFIX}/${_realname}"
else
  _qt4_prefix="${MINGW_PREFIX}"
fi

pkgver=4.8.7
pkgrel=5
arch=('any')
url='https://www.qt.io/'
install=qt4-${CARCH}.install
license=('GPL3' 'LGPL' 'FDL' 'custom')
conflicts=("${MINGW_PACKAGE_PREFIX}-qt5")
depends=("${MINGW_PACKAGE_PREFIX}-gcc-libs"
         "${MINGW_PACKAGE_PREFIX}-dbus"
         "${MINGW_PACKAGE_PREFIX}-fontconfig"
         "${MINGW_PACKAGE_PREFIX}-freetype"
         "${MINGW_PACKAGE_PREFIX}-libiconv"
         "${MINGW_PACKAGE_PREFIX}-libjpeg"
         "${MINGW_PACKAGE_PREFIX}-libmng"
         "${MINGW_PACKAGE_PREFIX}-libpng"
         "${MINGW_PACKAGE_PREFIX}-libtiff"
         "${MINGW_PACKAGE_PREFIX}-libwebp"
         "${MINGW_PACKAGE_PREFIX}-libxml2"
         "${MINGW_PACKAGE_PREFIX}-libxslt"
         "${MINGW_PACKAGE_PREFIX}-openssl"
         "${MINGW_PACKAGE_PREFIX}-pcre"
         "${MINGW_PACKAGE_PREFIX}-qtbinpatcher"
         "${MINGW_PACKAGE_PREFIX}-sqlite3"
         "${MINGW_PACKAGE_PREFIX}-zlib")
makedepends=("bison"
             "flex"
             "gperf"
             "lndir"
             "m4"
             "perl"
             "${MINGW_PACKAGE_PREFIX}-gcc"
             "${MINGW_PACKAGE_PREFIX}-firebird2"
             "${MINGW_PACKAGE_PREFIX}-libmariadbclient"
             "${MINGW_PACKAGE_PREFIX}-postgresql"
              "${MINGW_PACKAGE_PREFIX}-pkg-config"
             "${MINGW_PACKAGE_PREFIX}-python2"
             "${MINGW_PACKAGE_PREFIX}-ruby"
             "${MINGW_PACKAGE_PREFIX}-sqlite3"
             #"${MINGW_PACKAGE_PREFIX}-perl"
             #"${MINGW_PACKAGE_PREFIX}-gperf"
            )
optdepends=($([[ "$_variant" == "-shared" ]] && echo \
            "${MINGW_PACKAGE_PREFIX}-libmariadbclient" \
            "${MINGW_PACKAGE_PREFIX}-libfbclient" \
            "${MINGW_PACKAGE_PREFIX}-postgresql")
           )
groups=("${MINGW_PACKAGE_PREFIX}-qt4")
options=('!strip' 'staticlibs')
_pkgfqn="qt-everywhere-opensource-src-${pkgver}"
source=("https://download.qt.io/official_releases/qt/${pkgver%.*}/${pkgver}/${_pkgfqn}.tar.gz"
        0001-qt-4.6-demo-plugandpaint.patch
        0002-qt-4.8.0-fix-include-windows-h.patch
        0003-qt-4.8.0-fix-javascript-jit-on-mingw-x86_64.patch
        0004-qt-4.8.0-fix-mysql-driver-build.patch
        0005-qt-4.8.0-no-webkit-tests.patch
        0006-qt-4.8.0-use-fbclient-instead-gds32.patch
        0007-qt-4.8.1-fix-activeqt-compilation.patch
        0008-qt-4.8.3-assistant-4.8.2+gcc-4.7.patch
        0009-qt-4.8.3-qmake-cmd-mkdir-slash-direction.patch
        0010-qt-4.8.5-dont-set-qt-dll-define-for-static-builds.patch
        0011-qt4-merge-static-and-shared-library-trees.patch
        0012-qt4-use-correct-pkg-config-static-flags.patch
        0013-qt4-fix-linking-against-static-dbus.patch
        0014-qt-4.8.6-use-force-with-shell-commands.patch
        0015-qt-4-8-makefile_generator_gen_pkgconfig_fix_mingw.patch
        0016-qt-4.8.7-dont-add-resource-files-to-qmake-libs.patch
        0017-qt-4.8.7-mingw-configure.patch
        0018-qt-4.8.7-mingw-unix-tests.patch
        0019-qt-4.8.7-MinGW-w64-fix-qmng-define-MNG_USE_DLL-and-use-MNG_DECL.patch
        0020-qt-4.8.5-qmake-implib.patch
        0021-qt-4.8.7-fix-testcon-static-compile.patch
        0030-moc-boost-workaround.patch
        0031-qt4-gcc6.patch
        0032-qt4-icu59.patch
        0100-qt4-build-debug-qmake.patch)
sha256sums=('e2882295097e47fe089f8ac741a95fef47e0a73a3f3cdf21b56990638f626ea0'
            'e7c8fccf933dfee061b2960b5754e4835e7cb87c0e36166d3b11d69632732215'
            '5e6a61ced784d7d24c65d81e769b67b8f6066a33581c8b17cdf374a4c723cd23'
            'fba6cf2b0bd5f6229b21b1cf4adf093c216a10c97255527d71979599b7047be8'
            '002714db2e77c6a0c753d0caa9b63da0831dbb87cf542d660fd74d61aa12db90'
            '1e0eb85e1a88438ee1f99b5d27b775bf7d077bebf5de2e57fe831d0e39ce6df1'
            '8346737d3924598c1197202f8fcaa5cad4a5b81c2947797221357992b25260bd'
            '45cd7287a2b51a31a1e89831f4ac71aa933d504f05c780185319b91247116db5'
            '32e5e4cf899bbd4387c8c3633e51739b8e839ff2626b8386ea6153ae2408ead2'
            'ebd267ec82e718516cf8fb6d14035ae1b6e251d80ccef098f1514bd7599a9cb0'
            '1402443bca2ae19e983b8bbc3349c7c5d9f3f7ea53e88e339225737e76d38a64'
            '1f20dc102fa126f585e2fc84eb96bfe9ca617c4e475eb11f438ebcaa1cdce3d5'
            '723bcb7b17cf71bde47d223646de22b0dec715055b4a9306eb8fbf7023731f61'
            'c1084a627ec116c59ba91ed5729a7522ec82bb50457dc080161f414e6bb7d48f'
            'c9f09112daf40f5b896035d06eb71ead68db30197b8b4fcfbee6d3fcc895a7f4'
            'db6a5287c23e48c12bf1869eda7e9dd4482526df3202a78343697afa73a6ee5e'
            'e5d18b22410dedd75341edcaeb4cb32458c467d22459c754fb5c0ee711d4bb5b'
            '0bb41f9ddccbf5a649ffd73c1675ac3cbe7b05698abd875205ffe4f6e24913de'
            '00bc046d0c5de7bbb30d87c4fa76a0e699bb0f65bd262e5e4f4a753c46d6e757'
            'e0565cf9b3e54bbb030e478a93181314de88bebcd3bb25cbe8e597fbc5032e09'
            'e77b682e3728b835fe36d9dc48ab8c7472aed6d2ecf95a38219db95152602b32'
            'ddee93f387730551b5f8f6f468f97c5e6fd9b43a21458171ebd7ed0e9a909043'
            '62bc20838da22d832c550b9d38f98afb8fa4874915466945ea320575e264869a'
            '51da49e41edac66559d3ec8dd0a152995a51a53e5d1f63f09fa089a8af7e3112'
            '61d6bf45649c728dec5f8d22be5b496ed9d40f52c2c70102696d07133cd1750d'
            'e19a32b9dc050b3a605a6a5cb40fc3da8d8167ef240c105bb0737cc8bebadceb')

prepare() {
  cd ${srcdir}/${_pkgfqn}
  patch -p1 -i ${srcdir}/0001-qt-4.6-demo-plugandpaint.patch
  patch -p1 -i ${srcdir}/0002-qt-4.8.0-fix-include-windows-h.patch
  patch -p1 -i ${srcdir}/0003-qt-4.8.0-fix-javascript-jit-on-mingw-x86_64.patch
  patch -p1 -i ${srcdir}/0004-qt-4.8.0-fix-mysql-driver-build.patch
  patch -p1 -i ${srcdir}/0005-qt-4.8.0-no-webkit-tests.patch
  patch -p1 -i ${srcdir}/0006-qt-4.8.0-use-fbclient-instead-gds32.patch
  patch -p1 -i ${srcdir}/0007-qt-4.8.1-fix-activeqt-compilation.patch
  patch -p1 -i ${srcdir}/0008-qt-4.8.3-assistant-4.8.2+gcc-4.7.patch
  patch -p1 -i ${srcdir}/0009-qt-4.8.3-qmake-cmd-mkdir-slash-direction.patch
  patch -p1 -i ${srcdir}/0010-qt-4.8.5-dont-set-qt-dll-define-for-static-builds.patch
  patch -p1 -i ${srcdir}/0011-qt4-merge-static-and-shared-library-trees.patch
  patch -p1 -i ${srcdir}/0012-qt4-use-correct-pkg-config-static-flags.patch
  patch -p1 -i ${srcdir}/0013-qt4-fix-linking-against-static-dbus.patch
  patch -p1 -i ${srcdir}/0014-qt-4.8.6-use-force-with-shell-commands.patch
  patch -p1 -i ${srcdir}/0015-qt-4-8-makefile_generator_gen_pkgconfig_fix_mingw.patch
  patch -p1 -i ${srcdir}/0016-qt-4.8.7-dont-add-resource-files-to-qmake-libs.patch
  patch -p1 -i ${srcdir}/0017-qt-4.8.7-mingw-configure.patch
  patch -p1 -i ${srcdir}/0018-qt-4.8.7-mingw-unix-tests.patch
  patch -p1 -i ${srcdir}/0019-qt-4.8.7-MinGW-w64-fix-qmng-define-MNG_USE_DLL-and-use-MNG_DECL.patch
  patch -p2 -i ${srcdir}/0020-qt-4.8.5-qmake-implib.patch
  patch -p1 -i ${srcdir}/0021-qt-4.8.7-fix-testcon-static-compile.patch
  patch -p1 -i ${srcdir}/0030-moc-boost-workaround.patch
  patch -p1 -i ${srcdir}/0031-qt4-gcc6.patch
  patch -p1 -i ${srcdir}/0032-qt4-icu59.patch

  if check_option "debug" "y"; then
    patch -p1 -i ${srcdir}/0100-qt4-build-debug-qmake.patch
  fi

  #cd ${srcdir}
  #mkdir ${CARCH}
  #lndir ${srcdir}/${_pkgfqn} ${srcdir}/${CARCH}
  #cd ${CARCH}

  local _optim=
  case ${MINGW_CHOST} in
  i686*)
    _optim+=" -march=i686 -mtune=core2"
  ;;
  x86_64*)
    _optim+=" -march=nocona -mtune=core2"
  ;;
  esac

  local STATIC_LD=

  sed -i "s|^QMAKE_CFLAGS		=\(.*\)$|QMAKE_CFLAGS		= \1 ${_optim} |g" mkspecs/win32-g++/qmake.conf
  sed -i "s|^QMAKE_LFLAGS		=\(.*\)$|QMAKE_LFLAGS		= \1 ${STATIC_LD}|g" mkspecs/win32-g++/qmake.conf
  sed -i "s|^QMAKE_LIBS		=\(.*\)$|QMAKE_LIBS		=\1 -llcms2 -llzma|g" mkspecs/win32-g++/qmake.conf

  cd ${srcdir}
  mv ${_pkgfqn} ${_realname}-${pkgver}
}

build() {
  cd ${srcdir}/${_realname}-${pkgver}

  local _buildpkgdir=${pkgdirbase}/${pkgname}/${_qt4_prefix}
  mkdir -p ${_buildpkgdir}
  local QTDIR_WIN=$(cygpath -am ${_buildpkgdir})

  local pkg_conf_inc="-I ${MINGW_PREFIX}/include/mariadb"
  #pkg_conf_inc=$(pkg-config --dont-define-prefix --cflags-only-I libxml-2.0 dbus-1)
  export PATH=${srcdir}/${_realname}-${pkgver}/bin:${srcdir}/${_realname}-${pkgver}/lib:${PATH}

  export CXXFLAGS="$CXXFLAGS -std=gnu++98"
  ./configure \
    -prefix ${_buildpkgdir} \
    -datadir ${_buildpkgdir}/share/qt4 \
    -docdir ${_buildpkgdir}/share/qt4/doc \
    -headerdir ${_buildpkgdir}/include/qt4 \
    -plugindir ${_buildpkgdir}/share/qt4/plugins \
    -importdir ${_buildpkgdir}/share/qt4/imports \
    -translationdir ${_buildpkgdir}/share/qt4/translations \
    -sysconfdir ${_buildpkgdir}/etc \
    -examplesdir ${_buildpkgdir}/share/qt4/examples \
    -demosdir ${_buildpkgdir}/share/qt4/demos \
    -opensource \
    -confirm-license \
    -debug-and-release \
    -platform win32-g++ \
    -xplatform win32-g++ \
    -plugin-sql-ibase \
    -plugin-sql-mysql \
    -plugin-sql-psql \
    -plugin-sql-sqlite \
    -plugin-sql-odbc \
    -fontconfig \
    -openssl \
    -dbus \
    ${_variant} \
    -iconv \
    -qt3support \
    -optimized-qmake \
    -exceptions \
    -fast \
    -force-pkg-config \
    -little-endian \
    -xmlpatterns \
    -multimedia \
    -audio-backend \
    -phonon \
    -phonon-backend \
    -javascript-jit \
    -system-libmng \
    -system-libtiff \
    -system-libpng \
    -system-libjpeg \
    -system-sqlite \
    -system-zlib \
    -native-gestures \
    -s60 \
    -largefile \
    -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-neon \
    -nomake tests \
    -nomake demos \
    -nomake examples \
    -verbose \
    ${pkg_conf_inc}

  # Fix building
  #cp -rf mkspecs ${pkgdir}${MINGW_PREFIX}/

  make

}

package() {
  cd ${srcdir}/${_realname}-${pkgver}
  export PATH=${srcdir}/${CARCH}/bin:${PATH}
  make install
  rm -f ${pkgdir}${MINGW_PREFIX}/lib/*.dll

  if [ "${_variant}" = "-static" ]; then

    rm -rf ${pkgdir}${MINGW_PREFIX}/bin
    rm -rf ${pkgdir}${MINGW_PREFIX}/include
    rm -rf ${pkgdir}${MINGW_PREFIX}/share
    rm -rf ${pkgdir}${MINGW_PREFIX}/lib/pkgconfig
    rm  -f ${pkgdir}${MINGW_PREFIX}/lib/*.prl

  else

    plain "Install private headers"
    local PRIVATE_HEADERS=(
      phonon
      Qt3Support
      QtCore
      QtDBus
      QtDeclarative
      QtDesigner
      QtGui
      QtHelp
      QtMeeGoGraphicsSystemHelper
      QtMultimedia
      QtNetwork
      QtOpenGl
      QtOpenVG
      QtScript
      QtScriptTools
      QtSql
      QtSvg
      QtTest
      QtUiTools
      QtWebkit
      QtXmlPatterns
    )

    for priv_headers in ${PRIVATE_HEADERS[@]}
    do
      mkdir -p ${pkgdir}${MINGW_PREFIX}/include/Qt4/${priv_headers}
      mkdir -p ${pkgdir}${MINGW_PREFIX}/include/Qt4/${priv_headers}/private
    done

    plain "----> Qt3Support"
    cp -rfv `find src/qt3support -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/Qt3Support/private
    cp -rfv `find src/qt3support -type f -name "*_pch.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/Qt3Support/private

    plain "----> QtCore"
    cp -rfv `find src/corelib -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtCore/private
    cp -rfv `find src/corelib -type f -name "*_pch.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtCore/private

    plain "----> QtDBus"
    cp -rfv `find src/dbus -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtDBus/private

    plain "----> QtDeclarative"
    cp -rfv `find src/declarative -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtDeclarative/private

    plain "----> QtDesigner"
    cp -rfv `find tools/designer/src/lib -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtDesigner/private
    cp -rfv `find tools/designer/src/lib -type f -name "*_pch.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtDesigner/private

    plain "----> QtGui"
    cp -rfv `find src/gui -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtGui/private
    cp -rfv `find src/gui -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtGui/private

    plain "----> QtHelp"
    cp -rfv `find tools/assistant -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtHelp/private

    plain "----> QtMeeGoGraphicsSystemHelper"
    cp -rfv `find tools/qmeegographicssystemhelper -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtMeeGoGraphicsSystemHelper/private

    plain "----> QtMultimedia"
    cp -rfv `find src/multimedia -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtMultimedia/private

    plain "----> QtNetwork"
    cp -rfv `find src/network -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtNetwork/private

    plain "----> QtOpenGl"
    cp -rfv `find src/opengl -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtOpenGl/private

    plain "----> QtOpenVG"
    cp -rfv `find src/openvg -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtOpenVG/private

    plain "----> QtScript"
    cp -rfv `find src/script -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtScript/private

    plain "----> QtScriptTools"
    cp -rfv `find src/scripttools -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtScriptTools/private

    plain "----> QtSql"
    cp -rfv `find src/sql -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtSql/private

    plain "----> QtSvg"
    cp -rfv `find src/svg -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtSvg/private

    plain "----> QtTest"
    cp -rfv `find src/testlib -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtTest/private

    plain "----> QtUiTools"
    cp -rfv `find tools/designer/src/uitools -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtUiTools/private

    plain "----> QtWebkit"
    cp -rfv `find src/3rdparty/webkit -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtWebkit/private

    plain "----> QtXmlPatterns"
    cp -rfv `find src/xmlpatterns -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/QtXmlPatterns/private

    plain "----> phonon"
    cp -rfv `find src/3rdparty/phonon/phonon -type f -name "*_p.h"` ${pkgdir}${MINGW_PREFIX}/include/Qt4/phonon/private

    plain "---> Done install private headers"

    cp -rf lib/pkgconfig/* ${pkgdir}${MINGW_PREFIX}/lib/pkgconfig/
  fi

  #for tool in ${pkgdir}${MINGW_PREFIX}/bin/*.exe ; do
  #  mv ${pkgdir}${MINGW_PREFIX}/bin/${tool} ${pkgdir}${MINGW_PREFIX}/bin/${tool%.exe}-qt4.exe
  #done
}
