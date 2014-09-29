#!/usr/bin/env bash

set -e
set -x

# enable testing locally or on forks without multi-os enabled
if [[ "${TRAVIS_OS_NAME:-false}" == false ]]; then
    if [[ $(uname -s) == "Darwin" ]]; then
        TRAVIS_OS_NAME="osx"
    elif [[ $(uname -s) == "Linux" ]]; then
        TRAVIS_OS_NAME="linux"
    fi
fi


if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    lsb_release -a

    # Natron requires boost >= 1.49 to compile in C++11 mode
    # see http://stackoverflow.com/questions/11302758/error-while-copy-constructing-boostshared-ptr-using-c11
    # we use the irie/boost ppa for that purpose
    sudo add-apt-repository -y ppa:irie/boost
    # the PPA xorg-edgers contains cairo 1.12 (required for rotoscoping)
    sudo add-apt-repository -y ppa:xorg-edgers/ppa 
    if [ "$CC" = "gcc" ]; then sudo pip install cpp-coveralls --use-mirrors; fi
    # we get libyaml-cpp-dev from kubuntu backports (for OpenColorIO)
    if [ "$CC" = "gcc" ]; then sudo add-apt-repository -y ppa:kubuntu-ppa/backports; fi
    sudo apt-get update
    sudo apt-get update -qq

    sudo apt-get install libqt4-dev libglew-dev libboost-serialization-dev libexpat1-dev gdb libcairo2-dev
    # OpenFX
    if [ "$CC" = "gcc" ]; then make -C libs/OpenFX/Examples BITS=64; fi
    if [ "$CC" = "gcc" ]; then make -C libs/OpenFX/Support/Plugins BITS=64; fi
    if [ "$CC" = "gcc" ]; then make -C libs/OpenFX/Support/PropTester BITS=64; fi
    if [ "$CC" = "gcc" ]; then rm -rf Tests/Plugins; mkdir -p Tests/Plugins/Examples Tests/Plugins/Support Tests/Plugins/IO; fi
    if [ "$CC" = "gcc" ]; then mv libs/OpenFX/Examples/*/*-64-debug/*.ofx.bundle Tests/Plugins/Examples; fi
    if [ "$CC" = "gcc" ]; then mv libs/OpenFX/Support/Plugins/*/*-64-debug/*.ofx.bundle libs/OpenFX/Support/PropTester/*-64-debug/*.ofx.bundle Tests/Plugins/Support;  fi
    # OpenFX-IO
    if [ "$CC" = "gcc" ]; then sudo apt-get install cmake libtinyxml-dev liblcms2-dev libyaml-cpp-dev libboost-dev; git clone -b "v1.0.8" https://github.com/imageworks/OpenColorIO.git ocio; cd ocio; mkdir _build; cd _build; cmake .. -DCMAKE_INSTALL_PREFIX=/opt/ocio -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_JNIGLUE=OFF -DOCIO_BUILD_NUKE=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_STATIC_JNIGLUE=OFF -DOCIO_BUILD_TRUELIGHT=OFF -DUSE_EXTERNAL_LCMS=ON -DUSE_EXTERNAL_TINYXML=ON -DUSE_EXTERNAL_YAML=ON -DOCIO_BUILD_APPS=OFF -DOCIO_USE_BOOST_PTR=ON -DOCIO_BUILD_TESTS=OFF -DOCIO_BUILD_PYGLUE=OFF; make $MAKEFLAGSPARALLEL && sudo make install; cd ../..; fi
    if [ "$CC" = "gcc" ]; then sudo apt-get install libopenexr-dev libilmbase-dev; fi
    if [ "$CC" = "gcc" ]; then sudo apt-get install libopenjpeg-dev libtiff-dev libjpeg-dev libpng-dev libboost-filesystem-dev libboost-regex-dev libboost-thread-dev libboost-system-dev libwebp-dev libfreetype6-dev libssl-dev; git clone -b "RB-1.2" git://github.com/OpenImageIO/oiio.git oiio; cd oiio; make $MAKEFLAGSPARALLEL USE_QT=0 USE_TBB=0 USE_PYTHON=0 USE_FIELD3D=0 USE_OPENJPEG=1 USE_OCIO=1 OIIO_BUILD_TESTS=0 OIIO_BUILD_TOOLS=0 OCIO_HOME=/opt/ocio INSTALLDIR=/opt/oiio dist_dir=. cmake; sudo make $MAKEFLAGSPARALLEL dist_dir=.; cd ..; fi
    if [ "$CC" = "gcc" ]; then sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libavutil-dev; fi
    # config.pri
    # Ubuntu 12.04 precise doesn't have a pkg-config file for expat (expat.pc)
    echo 'boost: LIBS += -lboost_serialization' > config.pri
    echo 'expat: LIBS += -lexpat' >> config.pri
    echo 'expat: PKGCONFIG -= expat' >> config.pri
    echo 'sigar { ' >> config.pri
    echo 'INCLUDEPATH += /usr/local/include' >> config.pri
    echo 'LIBS += -L/usr/local/lib -lsigar' >> config.pri
    echo '}' >> config.pri

    # build OpenFX-IO
    if [ "$CC" = "gcc" ]; then (cd $TRAVIS_BUILD_DIR; git clone https://github.com/MrKepzie/openfx-io.git; (cd openfx-io; git submodule update --init --recursive)) ; fi
    if [ "$CC" = "gcc" ]; then env PKG_CONFIG_PATH=/opt/ocio/lib/pkgconfig make -C openfx-io BITS=64 OIIO_HOME=/opt/oiio; fi
    if [ "$CC" = "gcc" ]; then mv openfx-io/*/*-64-debug/*.ofx.bundle Tests/Plugins/IO;  fi

    #build  Sigar
    (cd $TRAVIS_BUILD_DIR; git clone https://github.com/hyperic/sigar.git; (cd sigar; ./autogen.sh; ./configure; make -C && sudo make install))

elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    sw_vers -productVersion

    install_xquartz(){
        XQUARTZ_VERSION=2.7.6

        wget --quiet http://xquartz.macosforge.org/downloads/SL/XQuartz-${XQUARTZ_VERSION}.dmg
        hdiutil mount XQuartz-${XQUARTZ_VERSION}.dmg
        sudo installer -store -pkg /Volumes/XQuartz-${XQUARTZ_VERSION}/XQuartz.pkg -target /
        hdiutil unmount /Volumes/XQuartz-${XQUARTZ_VERSION}
    }

    wget --quiet https://www.dropbox.com/s/7qwo3jzmdsugtis/bottles.tar &
    WGETPID=$!

    install_xquartz &

    brew update
    brew tap homebrew/python
    brew tap homebrew/science

    wait $WGETPID
    tar -xvf bottles.tar

    # official bottles
    brew install bottles/ilmbase-2.1.0.mavericks.bottle.tar.gz
    brew install bottles/openexr-2.1.0.mavericks.bottle.tar.gz
    #brew install bottles/cmake-2.8.12.2.mavericks.bottle.2.tar.gz
    brew install bottles/jasper-1.900.1.mavericks.bottle.tar.gz
    brew install bottles/little-cms2-2.6.mavericks.bottle.tar.gz
    #brew install bottles/doxygen-1.8.7.mavericks.bottle.tar.gz
    #brew install bottles/gmp-6.0.0a.mavericks.bottle.tar.gz
    #brew install bottles/mpfr-3.1.2-p8.mavericks.bottle.tar.gz
    #brew install bottles/libmpc-1.0.2.mavericks.bottle.tar.gz
    #brew install bottles/isl-0.12.2.mavericks.bottle.tar.gz
    #brew install bottles/cloog-0.18.1.mavericks.bottle.1.tar.gz
    brew install bottles/gcc-4.8.3_1.mavericks.bottle.tar.gz
    #brew install bottles/webp-0.4.0_1.mavericks.bottle.tar.gz
    brew install bottles/glew-1.10.0.mavericks.bottle.tar.gz
    brew install bottles/qt-4.8.6.mavericks.bottle.5.tar.gz
    brew install bottles/freetype-2.5.3_1.mavericks.bottle.1.tar.gz
    #brew install bottles/pcre-8.35.mavericks.bottle.tar.gz
    #brew install bottles/swig-3.0.2.mavericks.bottle.tar.gz
    brew install bottles/x264-r2412.mavericks.bottle.1.tar.gz
    brew install bottles/faac-1.28.mavericks.bottle.tar.gz
    brew install bottles/lame-3.99.5.mavericks.bottle.tar.gz
    brew install bottles/xvid-1.3.2.mavericks.bottle.tar.gz
    brew install bottles/ffmpeg-2.2.3.mavericks.bottle.tar.gz
    #brew install bottles/imagemagick-6.8.9-1.mavericks.bottle.tar.gz
    brew install bottles/libcaca-0.99b19.mavericks.bottle.tar.gz
    #brew install bottles/scons-2.3.1.mavericks.bottle.3.tar.gz

    # selfbuilt bottles
    brew install bottles/boost-1.55.0_2.mavericks.bottle.4.tar.gz
    brew install bottles/aces_container-1.0.mavericks.bottle.tar.gz
    brew install bottles/ctl-1.5.2.mavericks.bottle.tar.gz
    brew install bottles/jpeg-turbo-1.3.1.mavericks.bottle.tar.gz
    brew install bottles/libraw-0.15.4.mavericks.bottle.tar.gz
    #brew install bottles/seexpr-1.0.1.mavericks.bottle.tar.gz
    #brew install bottles/tbb-4.2.4.mavericks.bottle.tar.gz
    #brew install bottles/suite-sparse-4.2.1.mavericks.bottle.tar.gz
    #brew install bottles/numpy-1.8.1.mavericks.bottle.tar.gz
    brew install bottles/openjpeg-1.5.1_1.mavericks.bottle.tar.gz
    brew install bottles/cfitsio-3.360.mavericks.bottle.tar.gz
    #brew install bottles/szip-2.1.mavericks.bottle.tar.gz
    #brew install bottles/hdf5-1.8.13.mavericks.bottle.tar.gz
    brew install bottles/field3d-1.4.3.mavericks.bottle.tar.gz
    brew install bottles/opencolorio-1.0.9.mavericks.bottle.tar.gz
    brew install bottles/openimageio-1.4.8.mavericks.bottle.tar.gz
fi
