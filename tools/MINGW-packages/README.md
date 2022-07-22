# Natron Windows SDK

The following are instructions for installing and using the Natron SDK on Windows.

# Setup MSYS2

Download latest installer from [msys2.org](https://www.msys2.org/).

After installation start ``C:\msys64\mingw64.exe``.

```
pacman -Syu
pacman -S git
git clone https://github.com/NatronGitHub/Natron
cd Natron/tools/MINGW-packages
```

## Required packages

Several packages are required to use the SDK.

```
pacman -S `cat msys2.txt`
./get.sh
```

In some cases the `get.sh` command may provide incompatible packages, try building the failed package(s):

```
cd mingw-w64-PACKAGE
../makepkg.sh
pacman -U mingw-w64-x86_64-*-any.pkg.tar.*
```

or build everything:

```
./build.sh
```

# Setup a development environment (Qt Creator)

Start Qt Creator and go to ``Tools`` => ``Options``.

## Debuggers

Go to the ``Kits`` menu and select ``Debuggers``, then add a new.

* ``Name`` : ``MinGW GDB``
* ``Path`` : ``C:\msys64\mingw64\bin\gdb.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Compilers

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C``).

* ``Name`` : ``MinGW C``
* ``Path`` : ``C:\msys64\mingw64\bin\gcc.exe``

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C++``).

* ``Name`` : ``MinGW C++``
* ``Path`` : ``C:\msys64\mingw64\bin\g++.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Qt

Go to the ``Kits`` menu and select ``Qt Versions``, then add a new.

* ``Name`` : ``MinGW Qt 4.8.7``
* ``Path`` : ``C:\msys64\mingw64\bin\qmake.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Kit

Go to the ``Kits`` menu and select ``Kits``, then add a new.

* ``Name`` : ``Natron SDK Qt4``
* ``Compiler`` => ``C`` : ``MinGW C``
* ``Compiler`` => ``C++`` : ``MinGW C++``
* ``Debugger`` : ``MinGW GDB``
* ``Qt version`` : ``MinGW Qt 4.8.7``

## Project

Clone the Natron source (using Git Bash or MSYS2):

```
git clone https://github.com/NatronGitHub/Natron
cd Natron
git submodule update -i --recursive
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz
tar xvf Natron-v2.5.tar.gz
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
touch config.pri
```

Then start Qt Creator and open ``Project.pro`` from the Natron source folder and select the custom kit (``Natron MSYS2``) added previous. When Qt Creator is finished loading the project go to the ``Projects`` tab => ``Natron MSYS2`` => ``Build`` => ``Build Environment`` and add a new environment variable ``PYTHONHOME`` with the value ``C:\msys64\mingw64``.

You should now be able to build and run release and debug versions of Natron directly from Qt Creator.

# Setup a CI environment

With a working MSYS2 installation launch ``C:\msys64\mingw64.exe`` and clone the build tools:

```
git clone https://github.com/NatronGitHub/Natron NatronCI
```

Now create a build script:

```
#!/bin/bash
# Build Natron Windows Snapshot
#

export WORKSPACE=$HOME/NatronCI-workspace
export UNIT_TESTS=true
export BUILD_NAME=snapshots
export BUILD_NUMBER=RB-2.5
export GIT_URL=https://github.com/NatronGitHub/Natron.git
export SNAPSHOT_BRANCH=RB-2.5
export GIT_BRANCH=RB-2.5
export NATRON_LICENSE=GPL
export NATRON_DEV_STATUS=SNAPSHOT
export NATRON_BUILD_NUMBER=1
export COMPILE_TYPE=relwithdebinfo
export BITS=64
export DISABLE_BREAKPAD=1
export DISABLE_PORTABLE_ARCHIVE=0
export REMOTE_URL=NO_URL
export REMOTE_USER=NO_USER
export REMOTE_PREFIX=NO_PREFIX
export MKJOBS=4
export LOCALAPPDATA="/c/Users/USERNAME/AppData/Local"
export MSYSTEM=MINGW64
export NOUPDATE=1
export GIT_URL_IS_NATRON=1

if [ ! -d "$WORKSPACE" ]; then
    mkdir -p "$WORKSPACE"
fi

./launchBuildMain.sh
```

and save to ``$HOME/NatronCI/tools/jenkins/build-snapshot.sh``. Remember to change ``LOCALAPPDATA`` to match your username, and ``MKJOBS`` to match your CPU. You can disable unit tests with ``UNIT_TESTS=false``.

Now run the script *(working internet connection needed)*:

```
cd $HOME/NatronCI/tools/jenkins
bash build-snapshot.sh
```

The result will be in ``$HOME/NatronCI-workspace``.

This was just an example, see ``launchBuildMain.sh`` for more information/options.
