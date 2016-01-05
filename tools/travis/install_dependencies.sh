#!/usr/bin/env bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

# Exit immediately if a command exits with a non-zero status
set -e
# Print commands and their arguments as they are executed.
#set -x

# enable testing locally or on forks without multi-os enabled
if [[ "${TRAVIS_OS_NAME:-false}" == false ]]; then
    if [[ $(uname -s) == "Darwin" ]]; then
        TRAVIS_OS_NAME="osx"
    elif [[ $(uname -s) == "Linux" ]]; then
        TRAVIS_OS_NAME="linux"
    fi
fi


if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    TEST_CC=gcc
    lsb_release -a
    GCC_VERSION=4.9
    
    # Natron requires boost >= 1.49 to compile in C++11 mode
    # see http://stackoverflow.com/questions/11302758/error-while-copy-constructing-boostshared-ptr-using-c11
    ## we used the irie/boost ppa for that purpose
    #sudo add-apt-repository -y ppa:irie/boost
    # now we use ppa:boost-latest/ppa
    sudo add-apt-repository -y ppa:boost-latest/ppa
    # the PPA xorg-edgers contains cairo 1.12 (required for rotoscoping)
    sudo add-apt-repository -y ppa:xorg-edgers/ppa
    # ubuntu-toolchain-r/test contains recent versions of gcc
    if [ "$CC" = "$TEST_CC" ]; then sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test; sudo apt-get update; sudo apt-get install gcc-${GCC_VERSION} g++-${GCC_VERSION}; sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 90; sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} 90; fi

    if [ "$CC" = "$TEST_CC" ]; then sudo pip install cpp-coveralls; fi
    ## Python 3.4
    ##sudo add-apt-repository --yes ppa:fkrull/deadsnakes # python3.x
    # we get libyaml-cpp-dev from kubuntu backports (for OpenColorIO)
    if [ "$CC" = "$TEST_CC" ]; then sudo add-apt-repository -y ppa:kubuntu-ppa/backports; fi
    # we also need a recent ffmpeg for the newest version of the plugin
    #if [ "$CC" = "$TEST_CC" ]; then sudo add-apt-repository -y ppa:jon-severinsson/ffmpeg; fi #not available
    if [ "$CC" = "$TEST_CC" ]; then sudo add-apt-repository -y ppa:archivematica/externals; fi #2.5.1
    #if [ "$CC" = "$TEST_CC" ]; then sudo add-apt-repository -y ppa:pavlyshko/precise; fi #2.6.1
    sudo apt-get update
    sudo apt-get update -qq

    # Note: Python 3 packages are python3-dev and python3-pyside
    sudo apt-get install libqt4-dev libglew-dev libboost-serialization-dev libexpat1-dev gdb libcairo2-dev python-dev python-pyside libpyside-dev libshiboken-dev

    echo "*** Python version:"
    python --version
    python -c "from PySide import QtGui, QtCore, QtOpenGL"
    echo "*** PySide:"
    env PKG_CONFIG_PATH=`python-config --prefix`/lib/pkgconfig pkg-config --libs pyside
    echo "*** Shiboken:"
    pkg-config --libs shiboken
    cat /usr/lib/x86_64-linux-gnu/pkgconfig/shiboken.pc

    
    # OpenFX
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Examples; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Support/Plugins; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Support/PropTester; fi
    if [ "$CC" = "$TEST_CC" ]; then rm -rf Tests/Plugins; mkdir -p Tests/Plugins/Examples Tests/Plugins/Support Tests/Plugins/IO; fi
    if [ "$CC" = "$TEST_CC" ]; then mv libs/OpenFX/Examples/*/*-64-debug/*.ofx.bundle Tests/Plugins/Examples; fi
    if [ "$CC" = "$TEST_CC" ]; then mv libs/OpenFX/Support/Plugins/*/*-64-debug/*.ofx.bundle libs/OpenFX/Support/PropTester/*-64-debug/*.ofx.bundle Tests/Plugins/Support;  fi
    # OpenFX-IO
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install cmake libtinyxml-dev liblcms2-dev libyaml-cpp-dev libboost-dev libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libswresample-dev; wget https://github.com/imageworks/OpenColorIO/archive/v1.0.9.tar.gz -O /tmp/ocio-1.0.9.tar.gz; tar zxf /tmp/ocio-1.0.9.tar.gz; cd OpenColorIO-1.0.9; mkdir _build; cd _build; cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/ocio -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_JNIGLUE=OFF -DOCIO_BUILD_NUKE=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_STATIC_JNIGLUE=OFF -DOCIO_BUILD_TRUELIGHT=OFF -DUSE_EXTERNAL_LCMS=ON -DUSE_EXTERNAL_TINYXML=ON -DUSE_EXTERNAL_YAML=ON -DOCIO_BUILD_APPS=OFF -DOCIO_USE_BOOST_PTR=ON -DOCIO_BUILD_TESTS=OFF -DOCIO_BUILD_PYGLUE=OFF; make $J && make install; cd ../..; fi
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install libopenexr-dev libilmbase-dev; fi
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install libopenjpeg-dev libtiff-dev libjpeg-dev libpng-dev libboost-filesystem-dev libboost-regex-dev libboost-thread-dev libboost-system-dev libwebp-dev libfreetype6-dev libssl-dev; wget https://github.com/OpenImageIO/oiio/archive/Release-1.6.8.tar.gz -O /tmp/OpenImageIO-1.6.8.tar.gz; tar zxf /tmp/OpenImageIO-1.6.8.tar.gz; cd oiio-Release-1.6.8; make $J USE_QT=0 USE_TBB=0 USE_PYTHON=0 USE_PYTHON3=0 USE_FIELD3D=0 USE_FFMPEG=0 USE_OPENJPEG=1 USE_OCIO=1 USE_OPENCV=0 USE_OPENSSL=0 USE_FREETYPE=1 USE_GIF=0 USE_PTEX=0 USE_LIBRAW=0 OIIO_BUILD_TESTS=0 OIIO_BUILD_TOOLS=0 OCIO_HOME=$HOME/ocio INSTALLDIR=$HOME/oiio dist_dir=. cmake; make $J dist_dir=.; cd ..; fi
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libavutil-dev; fi
    if [ "$CC" = "$TEST_CC" ]; then wget https://github.com/wdas/SeExpr/archive/rel-1.0.1.tar.gz -O /tmp/SeExpr-1.0.1.tar.gz; tar zxf /tmp/SeExpr-1.0.1.tar.gz; cd SeExpr-rel-1.0.1; mkdir _build; cd _build; cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/seexpr; make $J && make install; cd ../..; fi
    # config.pri
    # Ubuntu 12.04 precise doesn't have a pkg-config file for expat (expat.pc)
    echo 'boost: LIBS += -lboost_serialization' > config.pri
    echo 'expat: LIBS += -lexpat' >> config.pri
    echo 'expat: PKGCONFIG -= expat' >> config.pri
    # pyside and shiboken for python3 cannot be configured with pkg-config on Ubuntu 12.04LTS Precise
    # pyside and shiboken for python2 still need the extra QtCore and QtGui include directories
    echo 'pyside: PKGCONFIG -= pyside' >> config.pri
    echo 'pyside: INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)' >> config.pri
    echo 'pyside: INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtCore' >> config.pri
    echo 'pyside: INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtGui' >> config.pri
    echo 'pyside: INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)' >> config.pri
    #echo 'pyside: LIBS += -lpyside.cpython-32mu' >> config.pri
    echo 'pyside: LIBS += -lpyside-python2.7' >> config.pri
    # pyside doesn't have PySide::getWrapperForQObject on Ubuntu 12.04LTS Precise 
    echo 'pyside: DEFINES += PYSIDE_OLD' >> config.pri
    #echo 'shiboken: PKGCONFIG -= shiboken' >> config.pri
    #echo 'shiboken: INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken)' >> config.pri
    #echo 'shiboken: LIBS += -lshiboken.cpython-32mu' >> config.pri

    # build OpenFX-IO
    if [ "$CC" = "$TEST_CC" ]; then (cd $TRAVIS_BUILD_DIR; git clone https://github.com/MrKepzie/openfx-io.git; (cd openfx-io; git submodule update --init --recursive)) ; fi
    if [ "$CC" = "$TEST_CC" ]; then env PKG_CONFIG_PATH=$HOME/ocio/lib/pkgconfig make -C openfx-io SEEXPR_HOME=$HOME/seexpr OIIO_HOME=$HOME/oiio; fi
    if [ "$CC" = "$TEST_CC" ]; then mv openfx-io/*/*-64-debug/*.ofx.bundle Tests/Plugins/IO;  fi

elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    TEST_CC=clang
    sw_vers -productVersion

    # See Travis OSX setup:
    # http://docs.travis-ci.com/user/osx-ci-environment

    # XQuartz already installed on Travis
    # install_xquartz(){
    #     echo "XQuartz start install"
    #     XQUARTZ_VERSION=2.7.6
    #     
    #     echo "XQuartz download"
    #     wget --quiet http://xquartz.macosforge.org/downloads/SL/XQuartz-${XQUARTZ_VERSION}.dmg
    #     echo "XQuartz mount dmg"
    #     hdiutil mount XQuartz-${XQUARTZ_VERSION}.dmg
    #     echo "XQuartz installer"  # sudo
    #     installer -store -pkg /Volumes/XQuartz-${XQUARTZ_VERSION}/XQuartz.pkg -target /
    #     echo "XQuartz unmount"
    #     hdiutil unmount /Volumes/XQuartz-${XQUARTZ_VERSION}
    #     echo "XQuartz end"
    # }

    # sudo install_xquartz &
    # XQ_INSTALL_PID=$!

    brew update
    brew tap homebrew/python
    brew tap homebrew/science

    echo "Install Natron dependencies"
    echo " - pip install numpy"
    pip install numpy
    # brew install numpy  # Compilation errors with gfortran
    echo " - install brew packages"
    # TuttleOFX's dependencies:
    #brew install scons swig ilmbase openexr jasper little-cms2 glew freetype fontconfig ffmpeg imagemagick libcaca aces_container ctl jpeg-turbo libraw seexpr openjpeg opencolorio openimageio
    # Natron's dependencies only
    brew install qt expat cairo glew
    # pyside/shiboken with python3 support take a long time to compile, see https://github.com/travis-ci/travis-ci/issues/1961
    #brew install pyside --with-python3 --without-python &
    #while true; do
    #    #ps -p$! 2>& 1>/dev/null
    #    #if [ $? = 0 ]; then
    #    if ps -p$! 2>& 1>/dev/null; then
    #      echo "still going"; sleep 10
    #    else
    #        break
    #    fi
    #done
    # Python 2 pyside comes precompiled!
    brew install python pyside shiboken
    if [ "$CC" = "$TEST_CC" ]; then
	# dependencies for building all OpenFX plugins
	brew install ilmbase openexr freetype fontconfig ffmpeg opencolorio openimageio seexpr
	# let OIIO work even if the package is not up to date (happened once, when hdf5 was upgraded to 5.10 but oiio was still using 5.9)
	hdf5lib=`otool -L /usr/local/lib/libOpenImageIO.dylib |fgrep hdf5 | awk '{print $1}'`
	if [ "$hdf5lib" -a ! -f "$hdf5lib" ]; then
	    ln -s libhdf5.dylib "$hdf5lib"
	fi
    fi

    PATH=/usr/local/bin:"$PATH"
    echo "Path: $PATH"
    ls -l /usr/local/bin/python
    echo "Python version:"
    type python
    python --version
    python -c "from PySide import QtGui, QtCore, QtOpenGL"
    echo "PySide libs:"
    env PKG_CONFIG_PATH=`python-config --prefix`/lib/pkgconfig pkg-config --libs pyside


    # OpenFX
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Examples; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Support/Plugins; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Support/PropTester; fi
    if [ "$CC" = "$TEST_CC" ]; then rm -rf Tests/Plugins; mkdir -p Tests/Plugins/Examples Tests/Plugins/Support Tests/Plugins/IO; fi
    if [ "$CC" = "$TEST_CC" ]; then mv libs/OpenFX/Examples/*/*-64-debug/*.ofx.bundle Tests/Plugins/Examples; fi
    if [ "$CC" = "$TEST_CC" ]; then mv libs/OpenFX/Support/Plugins/*/*-64-debug/*.ofx.bundle libs/OpenFX/Support/PropTester/*-64-debug/*.ofx.bundle Tests/Plugins/Support;  fi
    # OpenFX-IO
    if [ "$CC" = "$TEST_CC" ]; then (cd $TRAVIS_BUILD_DIR; git clone https://github.com/MrKepzie/openfx-io.git; (cd openfx-io; git submodule update --init --recursive)) ; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C openfx-io OIIO_HOME=/usr/local; fi
    if [ "$CC" = "$TEST_CC" ]; then mv openfx-io/*/*-64-debug/*.ofx.bundle Tests/Plugins/IO;  fi

    # wait $XQ_INSTALL_PID || true

    echo "End dependencies installation."
    # config.pri
    echo 'boost: INCLUDEPATH += /usr/local/include' > config.pri
    echo 'boost: LIBS += -L/usr/local/lib -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt' >> config.pri
    echo 'expat: PKGCONFIG -= expat' >> config.pri
    echo 'expat: INCLUDEPATH += /usr/local/opt/expat/include' >> config.pri
    echo 'expat: LIBS += -L/usr/local/opt/expat/lib -lexpat' >> config.pri
fi
#debug travis
pwd
ls
cat config.pri
echo "GCC/G++ versions:"
gcc --version
g++ --version
