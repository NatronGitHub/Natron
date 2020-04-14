#! /usr/bin/sh
# este script es para establecer el entorno de trabajo, solo para centos 7

# instalacion de dependencias
yum install epel-release
yum install fontconfig-devel gcc-c++ expat-devel python-pyside-devel shiboken-devel qt-devel boost169-devel pixman-devel cairo-devel
# -------------------

# Natron necesita gcc-c++ superior al 4.9, asi que necesitamos el 'devtoolset-8'
yum install centos-release-scl-rh
yum -y install devtoolset-8
# -----------------------

# link de la libreria boost 1.69
ln -s /usr/lib64/libboost_serialization.so.1.69.0 /usr/lib64/libboost_serialization.so
ln -s /usr/lib64/libboost_system.so.1.69.0 /usr/lib64/libboost_system.so
ln -s /usr/lib64/libboost_thread.so.1.69.0 /usr/lib64/libboost_thread.so
# ---------------------------
