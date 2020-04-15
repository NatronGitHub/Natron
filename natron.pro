TEMPLATE = subdirs

SUBDIRS += \
    HostSupport \
    gflags \
    glog \
    ceres \
    libmv \
    openMVG \
    qhttpserver \
    libtess \
    Engine \
    Renderer \
    Gui \
    PythonBin \
    App


# directorio donde estan los sub proyectos
gflags.subdir = libs/gflags
glog.subdir = libs/glog
ceres.subdir = libs/ceres
libmv.subdir = libs/libmv
openMVG.subdir = libs/openMVG
qhttpserver.subdir = libs/qhttpserver
libtess.subdir = libs/libtess
#-----------------------------

# que sub proyecto dependen del otro
glog.depends = gflags
ceres.depends = glog gflags
libmv.depends = gflags ceres
openMVG.depends = ceres
Engine.depends = libmv openMVG HostSupport libtess ceres
Renderer.depends = Engine
Gui.depends = Engine qhttpserver
App.depends = Gui Engine
#-----------------------------