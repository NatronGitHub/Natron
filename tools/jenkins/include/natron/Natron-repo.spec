# Custom spec for running on build server

%define _binary_payload w7.xzdio

Summary: Natron Repository
Name: Natron-repo

Version: 2
Release: 1
License: GPLv2

Group: System Environment/Base
URL: http://natron.fr
Vendor: INRIA, http://inria.fr
AutoReqProv: no
BuildArch: noarch

%description
Natron Repository

%build
echo OK

%install
mkdir -p $RPM_BUILD_ROOT/etc/yum.repos.d $RPM_BUILD_ROOT/etc/pki/rpm-gpg

cat __INC__/natron/Natron.repo > $RPM_BUILD_ROOT/etc/yum.repos.d/Natron.repo
cat __INC__/natron/GPG-KEY > $RPM_BUILD_ROOT/etc/pki/rpm-gpg/RPM-GPG-KEY-NATRON

%files
%defattr(-,root,root)
/etc/yum.repos.d/Natron.repo
/etc/pki/rpm-gpg/RPM-GPG-KEY-NATRON

%changelog
