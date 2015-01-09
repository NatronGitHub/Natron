#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e
# Print commands and their arguments as they are executed.
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
    TEST_CC=gcc
    lsb_release -a

    # Natron requires boost >= 1.49 to compile in C++11 mode
    # see http://stackoverflow.com/questions/11302758/error-while-copy-constructing-boostshared-ptr-using-c11
    # we use the irie/boost ppa for that purpose
    sudo add-apt-repository -y ppa:irie/boost
    # the PPA xorg-edgers contains cairo 1.12 (required for rotoscoping)
    sudo add-apt-repository -y ppa:xorg-edgers/ppa 
    if [ "$CC" = "$TEST_CC" ]; then sudo pip install cpp-coveralls --use-mirrors; fi
    # we get libyaml-cpp-dev from kubuntu backports (for OpenColorIO)
    if [ "$CC" = "$TEST_CC" ]; then sudo add-apt-repository -y ppa:kubuntu-ppa/backports; fi
    sudo apt-get update
    sudo apt-get update -qq

    sudo apt-get install libqt4-dev libglew-dev libboost-serialization-dev libexpat1-dev gdb libcairo2-dev

    # PySide
    # see https://stackoverflow.com/questions/24489588/how-can-i-install-pyside-on-travis/24545890#24545890
    pip install PySide--no-index --find-links https://parkin.github.io/python-wheelhouse/;
    # Travis CI servers use virtualenvs, so we need to finish the install by the following
    python ~/virtualenv/python${TRAVIS_PYTHON_VERSION}/bin/pyside_postinstall.py -install

    # OpenFX
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Examples; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Support/Plugins; fi
    if [ "$CC" = "$TEST_CC" ]; then make -C libs/OpenFX/Support/PropTester; fi
    if [ "$CC" = "$TEST_CC" ]; then rm -rf Tests/Plugins; mkdir -p Tests/Plugins/Examples Tests/Plugins/Support Tests/Plugins/IO; fi
    if [ "$CC" = "$TEST_CC" ]; then mv libs/OpenFX/Examples/*/*-64-debug/*.ofx.bundle Tests/Plugins/Examples; fi
    if [ "$CC" = "$TEST_CC" ]; then mv libs/OpenFX/Support/Plugins/*/*-64-debug/*.ofx.bundle libs/OpenFX/Support/PropTester/*-64-debug/*.ofx.bundle Tests/Plugins/Support;  fi
    # OpenFX-IO
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install cmake libtinyxml-dev liblcms2-dev libyaml-cpp-dev libboost-dev; git clone -b "v1.0.9" https://github.com/imageworks/OpenColorIO.git ocio; cd ocio; mkdir _build; cd _build; cmake .. -DCMAKE_INSTALL_PREFIX=/opt/ocio -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_JNIGLUE=OFF -DOCIO_BUILD_NUKE=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_STATIC_JNIGLUE=OFF -DOCIO_BUILD_TRUELIGHT=OFF -DUSE_EXTERNAL_LCMS=ON -DUSE_EXTERNAL_TINYXML=ON -DUSE_EXTERNAL_YAML=ON -DOCIO_BUILD_APPS=OFF -DOCIO_USE_BOOST_PTR=ON -DOCIO_BUILD_TESTS=OFF -DOCIO_BUILD_PYGLUE=OFF; make $J && sudo make install; cd ../..; fi
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install libopenexr-dev libilmbase-dev; fi
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install libopenjpeg-dev libtiff-dev libjpeg-dev libpng-dev libboost-filesystem-dev libboost-regex-dev libboost-thread-dev libboost-system-dev libwebp-dev libfreetype6-dev libssl-dev; git clone -b "RB-1.4" git://github.com/OpenImageIO/oiio.git oiio; cd oiio; make $J USE_QT=0 USE_TBB=0 USE_PYTHON=0 USE_FIELD3D=0 USE_OPENJPEG=1 USE_OCIO=1 OIIO_BUILD_TESTS=0 OIIO_BUILD_TOOLS=0 OCIO_HOME=/opt/ocio INSTALLDIR=/opt/oiio dist_dir=. cmake; sudo make $J dist_dir=.; cd ..; fi
    if [ "$CC" = "$TEST_CC" ]; then sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libavutil-dev; fi
    # config.pri
    # Ubuntu 12.04 precise doesn't have a pkg-config file for expat (expat.pc)
    echo 'boost: LIBS += -lboost_serialization' > config.pri
    echo 'expat: LIBS += -lexpat' >> config.pri
    echo 'expat: PKGCONFIG -= expat' >> config.pri

    # build OpenFX-IO
    if [ "$CC" = "$TEST_CC" ]; then (cd $TRAVIS_BUILD_DIR; git clone https://github.com/MrKepzie/openfx-io.git; (cd openfx-io; git submodule update --init --recursive)) ; fi
    if [ "$CC" = "$TEST_CC" ]; then env PKG_CONFIG_PATH=/opt/ocio/lib/pkgconfig make -C openfx-io OIIO_HOME=/opt/oiio; fi
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
    if [ "$CC" = "$TEST_CC" ]; then
	# Natron's dependencies for building all OpenFX plugins
	brew install qt expat cairo glew pyside --with-python3 ilmbase openexr freetype fontconfig ffmpeg opencolorio openimageio
    else
	# Natron's dependencies only
	brew install qt expat cairo glew pyside --with-python3
    fi

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
