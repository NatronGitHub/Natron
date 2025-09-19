# Natron-SDK ubuntu 24.04 (apt) dockerfile.
FROM ubuntu:24.04
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home
ARG SDK=/opt/Natron-sdk
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y build-essential llvm xorg-dev libgl-dev libegl1-mesa-dev libglu-dev curl wget git valgrind zip python3 python3-dev libosmesa6-dev libboost-serialization-dev libboost-system-dev libexpat1-dev libcairo2-dev qt5-qmake qtbase5-dev libshiboken2-dev libpyside2-dev python3-pyside2.qtwidgets python3-qtpy qtdeclarative5-dev extra-cmake-modules libopenexr-dev libilmbase-dev libavcodec-dev libraw-dev libopenimageio-dev libopencolorio-dev libmagick++-dev libavformat-dev libcdr-dev libpoppler-glib-dev libpoppler-private-dev libpoppler-qt5-dev libpango1.0-dev libzip-dev libssl-dev libfftw3-dev && rm -rf /var/lib/apt/lists/*
ENV WORKSPACE=/home \
    GIT_URL=https://github.com/NatronGitHub/Natron.git \
    GIT_BRANCH=RB-2.6 \
    GIT_COMMIT= \
    RELEASE_TAG=  \
    SNAPSHOT_BRANCH= \
    SNAPSHOT_COMMIT= \
    UNIT_TESTS=true \
    NATRON_LICENSE=GPL \
    DISABLE_BREAKPAD=1 \
    COMPILE_TYPE=release \
    NATRON_DEV_STATUS=RC \
    NATRON_CUSTOM_BUILD_USER_NAME= \
    NATRON_EXTRA_QMAKE_FLAGS= \
    BUILD_NAME=natron_github_RB2 \
    DISABLE_RPM_DEB_PKGS=1 \
    DISABLE_PORTABLE_ARCHIVE= \
    BITS= \
    DEBUG_SCRIPTS= \
    EXTRA_PYTHON_MODULES_SCRIPT= \
    BUILD_NUMBER=0

COPY \
    common.sh \
    compiler-common.sh \
    linuxStartupJenkins.sh \
    launchBuildMain.sh \
    manageBuildOptions.sh \
    manageLog.sh \
    createBuildOptionsFile.sh \
    gitRepositories.sh \
    checkout-repository.sh \
    build-plugins.sh \
    build-natron.sh \
    build-Linux-installer.sh \
    gen-natron-doc.sh \
    zip-python.sh \
    runUnitTests.sh \
    uploadArtifactsMain.sh \
    ./
COPY include/ include/
CMD ./launchBuildMain.sh
