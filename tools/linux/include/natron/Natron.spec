# Custom spec for running on build server

%define _binary_payload w7.xzdio

Summary: open-source compositing software
Name: Natron

Version: REPLACE_VERSION
Release: 1
License: GPLv2

Group: System Environment/Base
URL: http://natron.fr
Vendor: INRIA, http://inria.fr
AutoReqProv: no

%description
Node-graph based, open-source compositing software. Similar in functionalities to Adobe After Effects and The Foundry Nuke.

%build
echo OK

%install
mkdir -p $RPM_BUILD_ROOT/etc/yum.repos.d $RPM_BUILD_ROOT/usr/share/mime/packages $RPM_BUILD_ROOT/opt/Natron2 $RPM_BUILD_ROOT/usr/share/applications $RPM_BUILD_ROOT/usr/share/pixmaps $RPM_BUILD_ROOT/usr/bin

cp -a /root/Natron/tools/linux/tmp/Natron-installer/packages/fr.inria.*/data/* $RPM_BUILD_ROOT/opt/Natron2/
cat /root/Natron/tools/linux/include/natron/Natron2.desktop > $RPM_BUILD_ROOT/usr/share/applications/Natron2.desktop
cat /root/Natron/tools/linux/include/natron/x-natron.xml > $RPM_BUILD_ROOT/usr/share/mime/packages/x-natron.xml
cat /root/Natron/tools/linux/include/natron/Natron.repo > $RPM_BUILD_ROOT/etc/yum.repos.d/Natron.repo
cp /opt/Natron-CY2015/share/pixmaps/*.png $RPM_BUILD_ROOT/usr/share/pixmaps/

cd $RPM_BUILD_ROOT/usr/bin ; ln -sf ../../opt/Natron2/Natron .
cd $RPM_BUILD_ROOT/usr/bin; ln -sf ../../opt/Natron2/NatronRenderer .

%post -p /opt/Natron2/bin/postinstall.sh

%files
%defattr(-,root,root)
/etc/yum.repos.d/Natron.repo
/opt/Natron2
/usr/bin/Natron
/usr/bin/NatronRenderer
/usr/share/applications/Natron2.desktop
/usr/share/pixmaps/natronIcon256_linux.png
/usr/share/pixmaps/natronProjectIcon_linux.png
/usr/share/mime/packages/x-natron.xml

%changelog
