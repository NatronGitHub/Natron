#!/bin/bash

# Install perl (required to install XML:Parser perl module used by intltool)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/perl.html
PERL_VERSION=5.28.2
PERL_TAR="perl-${PERL_VERSION}.tar.gz"
PERL_SITE="http://www.cpan.org/src/5.0"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/perl" ]; }; }; then
    start_build
    download "$PERL_SITE" "$PERL_TAR"
    untar "$SRC_PATH/$PERL_TAR"
    pushd "perl-${PERL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./Configure -des -Dprefix="$SDK_HOME" -Dusethreads -Duseshrplib -Dcc="$CC"
    make -j${MKJOBS}
    make install
    PERL_MM_USE_DEFAULT=1 ${SDK_HOME}/bin/cpan -Ti XML::Parser
    popd
    rm -rf "perl-${PERL_VERSION}"
    end_build
fi
