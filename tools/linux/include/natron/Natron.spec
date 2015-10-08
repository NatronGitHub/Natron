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
mkdir -p $RPM_BUILD_ROOT/opt/Natron2 $RPM_BUILD_ROOT/usr/share/applications $RPM_BUILD_ROOT/usr/share/pixmaps $RPM_BUILD_ROOT/usr/bin

cp -a /root/Natron/tools/linux/tmp/Natron-installer/packages/fr.inria.*/data/* $RPM_BUILD_ROOT/opt/Natron2/
cat /root/Natron/tools/linux/include/natron/Natron2.desktop > $RPM_BUILD_ROOT/usr/share/applications/Natron2.desktop
cp /root/Natron/tools/linux/include/config/icon.png $RPM_BUILD_ROOT/usr/share/pixmaps/Natron.png

cd $RPM_BUILD_ROOT/usr/bin ; ln -sf ../../opt/Natron2/Natron .
cd $RPM_BUILD_ROOT/usr/bin; ln -sf ../../opt/Natron2/NatronRenderer .

%post -p /opt/Natron2/bin/postinstall.sh

%files
%defattr(-,root,root)
/opt/Natron2
/usr/bin/Natron
/usr/bin/NatronRenderer
/usr/share/applications/Natron2.desktop
/usr/share/pixmaps/Natron.png

%changelog
