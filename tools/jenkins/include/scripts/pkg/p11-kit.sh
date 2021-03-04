#!/bin/bash

# Install p11-kit (for gnutls)
# http://www.linuxfromscratch.org/blfs/view/svn/postlfs/p11-kit.html
P11KIT_VERSION=0.23.22 # select P11KIT_VERSION_SHORT below
P11KIT_VERSION_SHORT=${P11KIT_VERSION} # if P11KIT_VERSION has 3 components, eg 0.23.16
#P11KIT_VERSION_SHORT=${P11KIT_VERSION%.*} # if P11KIT_VERSION has 4 components, eg 0.23.16.1
P11KIT_TAR="p11-kit-${P11KIT_VERSION}.tar.xz"
P11KIT_SITE="https://github.com/p11-glue/p11-kit/releases/download/${P11KIT_VERSION}"
if download_step; then
    download "$P11KIT_SITE" "$P11KIT_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/p11-kit-1.pc" ] || [ "$(pkg-config --modversion p11-kit-1)" != "$P11KIT_VERSION_SHORT" ]; }; }; then
    start_build
    untar "$SRC_PATH/$P11KIT_TAR"
    pushd "p11-kit-${P11KIT_VERSION}"
    if [ $P11KIT_VERSION = 0.23.22 ]; then
        # https://github.com/p11-glue/p11-kit/commit/507c394cfcf4edffc5e4450c5d737e545c26b857
        patch -p1 -d. <<EOF
diff --git a/p11-kit/lists.c b/p11-kit/lists.c
index 365a6d89..1d9062be 100644
--- a/p11-kit/lists.c
+++ b/p11-kit/lists.c
@@ -39,6 +39,7 @@
 
 #include <assert.h>
 #include <ctype.h>
+#include <stdint.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
EOF
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "p11-kit-${P11KIT_VERSION}"
    end_build
fi
