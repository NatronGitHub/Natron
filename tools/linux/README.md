Natron on Linux
===============

Scripts used to build and distribute [Natron](http://www.natron.fr) on Linux.

Installation
============

 * Download http://mirror.nsc.liu.se/centos-store/6.4/isos/x86_64/CentOS-6.4-x86_64-minimal.iso (or http://mirror.nsc.liu.se/centos-store/6.4/isos/i386/CentOS-6.4-i386-minimal.iso)
 * Install ISO (remember to setup network)
 * Download https://github.com/MrKepzie/Natron/tools/linux/blob/master/include/scripts/setup-centos6.sh from another computer
 * Boot build server
 * Copy 'setup-centos6.sh' to server using SSH
 * login to build server as root
 * run 'setup-centos6.sh' (this may take a while)
 * git clone https://github.com/MrKepzie/Natron (in /root dir)

Online repository
==================

When building third-party dependencies or the Natron binaries you can upload them to a server. 
For this to work you need to create a file named **repo.sh** next to *snapshot.sh*, with for example the following content:

    #!/bin/sh

    REPO_DEST=user@host:/path
    REPO_URL=http://some.url

Launching snapshots
===================
	
    #Build GPL snapshots using 8 threads
    MKJOBS=8 NATRON_LICENSE=GPL sh snapshot.sh

Release build:
===============

To do a release build just edit the git tags accordingly in common.sh  then type:
```
rm -rf /opt/Natron-CY2015/bin/Natron* /opt/Natron-CY2015/Plugins /opt/Natron-CY2015/PyPlugs /opt/Natron-CY2015/share/OpenColorIO-Configs
TAR_BUILD=1 DEB_BUILD=1 RPM_BUILD=1 BUILD_CONFIG=STABLE BUILD_NUMBER=1 NATRON_LICENSE=GPL OFFLINE_INSTALLER=1 SYNC=1 NOCLEAN=1 CV=0 sh build.sh tag 4
```
