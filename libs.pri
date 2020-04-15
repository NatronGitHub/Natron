
libmv-flags {
CONFIG += ceres-flags
INCLUDEPATH += $$PWD/libs/libmv/third_party
INCLUDEPATH += $$PWD/libs/libmv
}

glad-flags {
    CONFIG(debug, debug|release):   INCLUDEPATH += $$PWD/Global/gladDeb/include
    CONFIG(release, debug|release): INCLUDEPATH += $$PWD/Global/gladRel/include
}

openmvg-flags {
CONFIG += ceres-flags boost

CONFIG += openmp

openmp {
    DEFINES += OPENMVG_USE_OPENMP
}

DEFINES += OPENMVG_HAVE_BOOST

# Do not use any serialization in openmvg (cereal, ply, stlplus ...)
DEFINES += OPENMVG_NO_SERIALIZATION

# Use this to use OsiMskSolverInterface.cpp
#DEFINES += OPENMVG_HAVE_MOSEK

c++11 {
   DEFINES += OPENMVG_STD_UNORDERED_MAP
   DEFINES += OPENMVG_STD_SHARED_PTR
} else {
   # Use boost::shared_ptr and boost::unordered_map
   CONFIG += boost
   DEFINES += OPENMVG_BOOST_UNORDERED_MAP
   DEFINES += OPENMVG_BOOST_SHARED_PTR
   DEFINES += OPENMVG_NO_UNIQUE_PTR
}

INCLUDEPATH += $$PWD/libs/openMVG/openMVG
INCLUDEPATH += $$PWD/libs/openMVG
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/Clp/src
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/Clp/src/OsiClip
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/CoinUtils/src
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/Osi/src/Osi
INCLUDEPATH += $$PWD/libs/flann/src/cpp
#INCLUDEPATH += $$PWD/libs/lemon

}

ceres-flags {
CONFIG += glog-flags
DEFINES += CERES_HAVE_PTHREAD CERES_NO_SUITESPARSE CERES_NO_CXSPARSE CERES_HAVE_RWLOCK
# Comment to make ceres use a lapack library
DEFINES += CERES_NO_LAPACK
CONFIG += openmp

openmp {
    DEFINES += CERES_USE_OPENMP
}
#If undefined, make sure to add to sources all the files in ceres/internal/ceres/generated
DEFINES += CERES_RESTRICT_SCHUR_SPECIALIZATION
DEFINES += WITH_LIBMV_GUARDED_ALLOC GOOGLE_GLOG_DLL_DECL= LIBMV_NO_FAST_DETECTOR=
c++11 {
   DEFINES += CERES_STD_UNORDERED_MAP
   DEFINES += CERES_STD_SHARED_PTR
} else {
   # Use boost::shared_ptr and boost::unordered_map
   CONFIG += boost
   DEFINES += CERES_BOOST_SHARED_PTR
   DEFINES += CERES_BOOST_UNORDERED_MAP
}
INCLUDEPATH += $$PWD/libs/ceres/config
INCLUDEPATH += $$PWD/libs/ceres/include
INCLUDEPATH += $$PWD/libs/ceres/internal
INCLUDEPATH += $$PWD/libs/Eigen3

}

glog-flags {
CONFIG += gflags-flags
DEFINES += GOOGLE_GLOG_DLL_DECL=
INCLUDEPATH += $$PWD/libs/gflags
INCLUDEPATH += $$PWD/libs/gflags/src
INCLUDEPATH += $$PWD/libs/gflags/src/gflags
INCLUDEPATH += $$PWD/libs/glog/src

INCLUDEPATH += $$PWD/libs/glog/src


}

gflags-flags {
INCLUDEPATH += $$PWD/libs/gflags
INCLUDEPATH += $$PWD/libs/gflags/src
INCLUDEPATH += $$PWD/libs/gflags/src/gflags
}

libtess-flags {
INCLUDEPATH += $$PWD/libs/libtess
}

################
# Gui
static-gui {
CONFIG += static-engine static-qhttpserver static-hoedown static-libtess
LIBS += -L$$OUT_PWD/../Gui/ -lGui


INCLUDEPATH += $$PWD/Gui
DEPENDPATH += $$OUT_PWD/../Gui

PRE_TARGETDEPS += $$OUT_PWD/../Gui/libGui.a



}  #static-gui

################
# Engine

static-engine {
CONFIG += static-libmv static-openmvg static-hoedown static-libtess

LIBS += -L$$OUT_PWD/../Engine/ -lEngine


INCLUDEPATH += $$PWD/Engine
DEPENDPATH += $$OUT_PWD/../Engine
INCLUDEPATH += $$PWD/libs/SequenceParsing
INCLUDEPATH += $$PWD/Global
PRE_TARGETDEPS += $$OUT_PWD/../Engine/libEngine.a

} #static-engine

################
# HostSupport

static-host-support {


LIBS += -L$$OUT_PWD/../HostSupport/ -lHostSupport


INCLUDEPATH += $$PWD/HostSupport
DEPENDPATH += $$OUT_PWD/../HostSupport
#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/libs/OpenFX/include
INCLUDEPATH += $$PWD/libs/OpenFX_extensions
INCLUDEPATH += $$PWD/libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD

PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/libHostSupport.a


} #static-host-support

################
# LibMV
static-libmv {
CONFIG += static-ceres
INCLUDEPATH += $$PWD/libs/libmv
DEPENDPATH += $$OUT_PWD/../libs/libmv

LIBS += -L$$OUT_PWD/../libs/libmv/ -lLibMV
PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/libLibMV.a


} # static-libmv


################
# openMVG
static-openmvg {
CONFIG += static-ceres
INCLUDEPATH += $$PWD/libs/openMVG
DEPENDPATH += $$OUT_PWD/../libs/openMVG
LIBS += -L$$OUT_PWD/../libs/openMVG/ -lopenMVG
PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/libopenMVG.a



} # static-openmvg

################
# ceres
static-ceres {
CONFIG += static-glog

LIBS += -L$$OUT_PWD/../libs/ceres/ -lceres
PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/libceres.a
CONFIG += openmp


} # static-ceres {

################
# glog
static-glog {
CONFIG += static-gflags
LIBS += -L$$OUT_PWD/../libs/glog/ -lglog
PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/libglog.a
} # static-glog {

################
# gflags
static-gflags {

LIBS += -L$$OUT_PWD/../libs/gflags/ -lgflags  
PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/libgflags.a  

} # static-gflags {

################
# qhttpserver

static-qhttpserver {

DEFINES += QHTTP_SERVER_STATIC
INCLUDEPATH += $$PWD/libs/qhttpserver/src
DEPENDPATH += $$OUT_PWD/../libs/qhttpserver/src
LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/ -lqhttpserver
PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/libqhttpserver.a


} # static-qhttpserver

################
# hoedown

static-hoedown {

INCLUDEPATH += $$PWD/libs/hoedown/src
DEPENDPATH += $$OUT_PWD/../libs/hoedown

LIBS += -L$$OUT_PWD/../libs/hoedown/build/ -lhoedown
PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/libhoedown.a


} # static-hoedown


################
# libtess

static-libtess {

INCLUDEPATH += $$PWD/libs/libtess
DEPENDPATH += $$OUT_PWD/../libs/libtess

LIBS += -L$$OUT_PWD/../libs/libtess/ -ltess
PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/libtess.a




} # static-libtess
