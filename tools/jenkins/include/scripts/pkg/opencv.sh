#!/bin/bash

exit 1

# Install opencv
CV_TAR=opencv-2.4.11.zip
if [ ! -s $SDK_HOME/lib/pkgconfig/opencv.pc ]; then
   pushd "$TMP_PATH"
   if [ ! -s $CWD/src/$CV_TAR ]; then
       $WGET $THIRD_PARTY_SRC_URL/$CV_TAR -O $CWD/src/$CV_TAR
   fi
   unzip $CWD/src/$CV_TAR
   cd opencv*
   mkdir build
   cd build
   env CFLAGS="$BF" CXXFLAGS="$BF" CMAKE_INCLUDE_PATH=$SDK_HOME/include CMAKE_LIBRARY_PATH=$SDK_HOME/lib cmake .. -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_FFMPEG=OFF -DWITH_OPENEXR=OFF -DWITH_OPENCL=OFF -DWITH_OPENGL=ON -DBUILD_WITH_DEBUG_INFO=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DENABLE_SSE3=OFF -DCMAKE_INSTALL_PREFIX=$SDK_HOME -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
   make -j${MKJOBS}
   make install
   mkdir -p $SDK_HOME/docs/opencv
   cp ../LIC* ../COP* ../README ../AUTH* ../CONT* $SDK_HOME/docs/opencv/
fi
