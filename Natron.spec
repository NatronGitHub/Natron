Summary: Open source compositing software
Name: Natron
# The two actively maintained versions (that are merged into master)
Version21: 2.1.10
Version22: 2.2.10
Version23: 2.3.15
Version30: 3.0.0

# The version for this branch of the sources
Version: %{version30}

# The release number (must be incremented whenever changes to this file generate different binaries)
Release: 1%{?dist}
License: GPLv2

Group: System Environment/Base
URL: https://natrongithub.github.io/

# https://github.com/NatronGitHub/Natron/releases/download/%{version}/Natron-%{version}.tar.xz
Source0: %{name}-%{version}.tar.xz
# https://github.com/NatronGitHub/Natron/releases/download/2.1.0/Natron-OpenColorIO-Configs-2.1.0.tar.xz
Source1: %{name}-OpenColorIO-Configs-2.1.0.tar.xz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: fontconfig-devel gcc-c++ expat-devel python-pyside-devel shiboken-devel qt-devel boost-devel pixman-devel glfw-devel cairo-devel
Requires: fontconfig qt-x11 python-pyside shiboken-libs boost-serialization pixman glfw cairo

%description
Open source compositing software. Node-graph based. Similar in functionalities to Adobe After Effects and Nuke by The Foundry.

%prep
%setup
%setup -T -D -a 1

%build
mv Natron-OpenColorIO-Configs-2.1.0 OpenColorIO-Configs
cat << 'EOF' > config.pri
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
PKGCONFIG += expat
PKGCONFIG += fontconfig
cairo {
        PKGCONFIG += cairo
        LIBS -=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
}
pyside {
        PKGCONFIG -= pyside
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtCore
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtGui
        INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
        LIBS += -lpyside-python2.7
}
shiboken {
        PKGCONFIG -= shiboken
        INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken)
        LIBS += -lshiboken-python2.7
}
EOF

mkdir build
cd build
qmake-qt4 -r ../Project.pro PREFIX=/usr CONFIG+=release DEFINES+=QT_NO_DEBUG_OUTPUT
make %{?_smp_mflags}

%install
cd build
make INSTALL_ROOT=%{buildroot} install

%clean
%{__rm} -rf %{buildroot}

%post
update-mime-database /usr/share/mime
update-desktop-database /usr/share/applications

%postun
update-mime-database /usr/share/mime
update-desktop-database /usr/share/applications

%files
%defattr(-,root,root,-)
/usr/bin/Natron
/usr/bin/NatronRenderer
/usr/share/applications/Natron.desktop
/usr/share/pixmaps/natronIcon256_linux.png
/usr/share/pixmaps/natronProjectIcon_linux.png
/usr/share/mime/packages/x-natron.xml
/usr/share/OpenColorIO-Configs
%doc LICENSE.txt

%changelog
