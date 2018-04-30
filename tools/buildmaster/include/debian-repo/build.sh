#!/bin/sh
CWD=`pwd`
TMP=$CWD/natron-repo
rm -f *.deb
rm -rf $TMP
mkdir -p $TMP
cd $TMP || exit 1

mkdir -p DEBIAN etc/apt/sources.list.d etc/apt/trusted.gpg.d usr/share/doc/natron-repo || exit 1

DEB_ARCH=all
DEB_VERSION=2
DEB_DATE=$(date +"%a, %d %b %Y %T %z")
DEB_PKG=natron-repo_${DEB_VERSION}_${DEB_ARCH}.deb
cp -a $CWD/debian/post* DEBIAN/ || exit 1
chmod +x DEBIAN/post*

cat $CWD/natron.list > etc/apt/sources.list.d/natron.list || exit 1
gpg --no-default-keyring --keyring etc/apt/trusted.gpg.d/natron.gpg --import $CWD/natron.key || exit 1
rm -f etc/apt/trusted.gpg.d/natron.gpg~

DEB_SIZE=$(du -ks .|cut -f 1)

cat $CWD/debian/copyright > usr/share/doc/natron-repo/copyright || exit 1
cat $CWD/debian/control | sed "s/__VERSION__/${DEB_VERSION}/;s/__ARCH__/${DEB_ARCH}/;s/__SIZE__/${DEB_SIZE}/" > DEBIAN/control || exit 1
cat $CWD/debian/changelog.Debian |sed "s/__VERSION__/${DEB_VERSION}/;s/__DATE__/${DEB_DATE}/" > changelog.Debian || exit 1
gzip changelog.Debian || exit 1
mv changelog.Debian.gz usr/share/doc/natron-repo/ || exit 1
chown root:root -R $TMP

cd $CWD || exit 1
dpkg-deb -Zxz -z9 --build natron-repo || exit 1
mv natron-repo.deb $DEB_PKG || exit 1
