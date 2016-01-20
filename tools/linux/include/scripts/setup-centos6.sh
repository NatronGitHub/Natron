#!/bin/sh

rm -f /etc/yum.repos.d/CentOS-Base.repo || exit 1
sed -i 's#baseurl=file:///media/CentOS/#baseurl=http://vault.centos.org/6.4/os/$basearch/#;s/enabled=0/enabled=1/;s/gpgcheck=1/gpgcheck=0/;/file:/d' /etc/yum.repos.d/CentOS-Media.repo || exit 1
yum -y install gettext make wget glibc-devel glibc-devel.i686 vixie-cron crontabs unzip rsync git screen file tree bc gcc-c++ kernel-devel libX*devel *GL*devel *xcb*devel xorg*devel libdrm-devel mesa*devel *glut*devel dbus-devel xz patch bison flex libtool-ltdl-devel || exit 1

# dont include png-dev
yum -y remove libpng-devel

chkconfig crond on
service crond start

echo "All done"
exit 0

