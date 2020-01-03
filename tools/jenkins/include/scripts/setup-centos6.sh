#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

WGET="wget --referer https://natron.fr/"

echo "WARNING!!! Only run this on a clean CentOS 6.4 minimal install"
sleep 10


rm -f /etc/yum.repos.d/CentOS-Base.repo
sed -i 's#baseurl=file:///media/CentOS/#baseurl=http://vault.centos.org/6.4/os/$basearch/#;s/enabled=0/enabled=1/;s/gpgcheck=1/gpgcheck=0/;/file:/d' /etc/yum.repos.d/CentOS-Media.repo
yum -y install zip gettext make wget glibc-devel glibc-devel.i686 vixie-cron crontabs unzip rsync git screen file tree bc gcc-c++ kernel-devel "libX*devel" "*GL*devel" "*xcb*devel" "xorg*devel" xorg-x11-fonts-Type1 libdrm-devel "mesa*devel" "*glut*devel" xz patch bison flex libtool-ltdl-devel texinfo || exit 

# epel repo for dpkg

cat <<'EOF' > "/etc/yum.repos.d/epel.repo"
[epel]
name=Extra Packages for Enterprise Linux 6 - $basearch
baseurl=http://download.fedoraproject.org/pub/epel/6/$basearch
failovermethod=priority
enabled=1
gpgcheck=0
EOF

# add sphinx
RPMS="python-docutils-0.11-0.2.20130715svn7687.el6.noarch.rpm  python-pygments-1.4-9.el6.noarch.rpm python-jinja2-2.7.2-2.el6.noarch.rpm python-sphinx-1.1.3-8.el6.noarch.rpm"
for i in $RPMS; do
    $WGET http://downloads.natron.fr/Third_Party_Binaries/$i
done
for x in $RPMS; do
    yum -y install $x
done

# don't include png-dev
yum -y remove libpng-devel
yum -y remove libjpeg-turbo-devel

# Update git to a version > 2, see:
# - https://stackoverflow.com/questions/21820715/how-to-install-latest-version-of-git-on-centos-6-x-7-x/38133865#38133865
yum remove git
yum install epel-release
rpm -Uvh  https://centos6.iuscommunity.org/ius-release.rpm
yum install git2u

# Update libcurl, see :
# - https://stackoverflow.com/a/36855672/2607517
# - https://www.digitalocean.com/community/questions/how-to-upgrade-curl-in-centos6
rpm -Uvh http://nervion.us.es/city-fan/yum-repo/rhel6/x86_64/city-fan.org-release-1-13.rhel6.noarch.rpm
yum upgrade

chkconfig crond on
service crond start

echo "All done"
exit 0


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
