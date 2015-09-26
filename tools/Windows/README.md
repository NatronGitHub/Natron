Natron on Windows
==================

Scripts used to build and distribute [Natron](http://www.natron.fr) from Windows to Windows using MingW-w64 (via MSYS2).

Build server installation
=========================

Requires any Windows machine (XP+).
These scripts make use of [MSYS2](https://sourceforge.net/projects/msys2/) to operate.  

Create the local.sh file in the root of natron-mingw to specify various infos, e.g:

    #!/bin/sh

    REPO_DEST=user@host:/path
    REPO_URL=http://some.url
    
Clone the [MINGW-packages](https://github.com/MrKepzie/MINGW-packages) repository next to snapshot.sh:

    cd tools/Windows
    git clone https://github.com/MrKepzie/MINGW-packages

If this is the first time installing it, make sure to install all base packages by running:
	
	#Use BIT=32 or BIT=64
	BIT=64
	sh include/scripts/setup-msys.sh $BIT
	sh include/scripts/build-sdk.sh $BIT
	
The environment is now ready to build Natron and plug-ins, just run:
	
	#Use BIT=32 or BIT=64
	BIT=64
	sh snapshots.sh 64

To launch a release build, edit common.sh to adjust which git tags should be used then:
	#GPL build, without openfx-opencv plug-ins for 64bit target using 8 threads.
	NATRON_LICENSE=GPL OFFLINE_INSTALLER=1 SYNC=1 NOCLEAN=1 CV=0 sh build.sh 64 tag 8