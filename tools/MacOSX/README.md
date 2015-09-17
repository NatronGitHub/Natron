# Natron on MacOSX

Scripts used to build and distribute [Natron](http://www.natron.fr) on MacOSX.


## Build server installation : basic setup

The first step is to install MacPorts. If you want to distribute an universal binary that runs on any OS X version >= 10.6, 

### Installing MacPorts on OS X 10.6 Snow Leopard (universal)

* Download and install MacOSX Snow Leopard 10.6 retail DVD.

* Install [Xcode 3.2.6 for Snow Leopard](https://guide.macports.org/#installing.xcode.snowleopard). Don't forget to also install the UNIX command-line utilities (this is an install option in Xcode 3).

* Install [MacPorts](https://www.macports.org/install.php) 

* Add the following line to /opt/local/etc/macports/variants.conf  (you may use `sudo nano /opt/local/etc/macports/variants.conf`):
```
-x11 +no_x11 +bash_completion +no_gnome +quartz +universal +llvm34 +ld64_236 +natron
```
  * `-x11 +no_x11 +no_gnome` are here to avoid linking to X11 libraries
  * `+llvm34` is to avoid ising llvm- 3.5 or greater, which require libc++
  * `+ld64_236` is to use the last version of ld64 that supports libstdc++
  * `+natron` is to build the Natro-specific version of ImageMagick 

### Installing MacPorts on OS X 10.7+ for deployment on OS X 10.6 (universal)

* Install the latest version of [Xcode](https://guide.macports.org/#installing.xcode)

* Download and install the missing OS X SDK, following instructions at https://github.com/devernay/xcodelegacy

* Check that you can compile a simple C++ program (Xcode 7 may fail here)
  * Create the file `conftest.cpp` containing:
```
int main () { return 0; }
```
  * Compile it with
```
g++ -mmacosx-version-min=10.6 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk -Wl,-syslibroot,/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk conftest.cpp -o conftest
```

* Install [MacPorts](https://www.macports.org/install.php) 

* Add the following lines to `/opt/local/etc/macports/macports.conf` (you may use `sudo nano /opt/local/etc/macports/macports.conf`):
```
macosx_deployment_target 10.6
macosx_sdk_version 10.6
sdkroot /Developer/SDKs/MacOSX10.6.sdk
cxx_stdlib         libstdc++
buildfromsource    always
```

* Add the following line to `/opt/local/etc/macports/variants.conf` (you may use `sudo nano /opt/local/etc/macports/variants.conf`):
```
-x11 +no_x11 +bash_completion +no_gnome +quartz +universal +llvm34 +ld64_236 +natron
```
  * `-x11 +no_x11 +no_gnome` are here to avoid linking to X11 libraries
  * `+llvm34` is to avoid ising llvm 3.5 or greater, which require libc++
  * `+ld64_236` is to use the last version of ld64 that supports libstdc++
  * `+natron` is to build the Natro-specific version of ImageMagick 

* Execute `sudo port selfupdate`

* Also apply patches from http://trac.macports.org/ticket/41783, namely:
  * http://trac.macports.org/raw-attachment/ticket/41783/portconfigure.tcl.3.diff (file is `/opt/local/libexec/macports/lib/port1.0/portconfigure.tcl`)
  * http://trac.macports.org/attachment/ticket/41783/cmake-1.0.tcl.diff (file is `/opt/local/var/macports/sources/rsync.macports.org/release/tarballs/ports/_resources/port1.0/group/cmake-1.0.tcl`, patch may have to be applied after each `port selfupdate`)
  
### Installing MacPorts on any version of OS X for deployment on the same OS X version

* Install the latest version of Xcode

* Install [MacPorts](https://www.macports.org/install.php) 

* Add the following line to /opt/local/etc/macports/variants.conf  (you may use `sudo nano /opt/local/etc/macports/variants.conf`):
```
-x11 +no_x11 +bash_completion +no_gnome +quartz +natron
```
  * `-x11 +no_x11 +no_gnome` are here to avoid linking to X11 libraries
  * `+natron` is to build the Natro-specific version of ImageMagick
  
##  Installing required packages

* Download Macports [dports-dev](http://downloads.natron.fr/Third_Party_Sources/dports-dev.zip)

* Give read/execute permissions to the local repository (replace `USER_NAME` with your user name):
```
chmod 755 /Users/USER_NAME/Development/dports-dev
```

* Check that the user "nobody" can read this directory by typing the following command in a Terminal: `sudo -u nobody ls /Users/USER_NAME/Development/dports-dev`. If it fails, then try again after having given execution permissions on your home directory using the following command: `chmod o+x /Users/USER_NAME`. If this still fails, then something is really wrong.

* Edit the sources.conf file for MacPorts, for example using the nano editor: `sudo nano /opt/local/etc/macports/sources.conf`, insert at the beginning of the file the configuration for a local repository (read the comments in the file), by inserting the line `file:///Users/USER_NAME/Development/dports-dev` (without quotes, and yes there are *three* - 3 - slashes). Save and exit (if you're using nano, this means typing ctrl-X, Y and return).

* Update MacPorts:
```
sudo port selfupdate
```

* Recreate the index in the local repository: (no need to be root for this)
```
cd /Users/USER_NAME/Development/dports-dev; portindex"
```

* *Only if the target is OS X 10.6 or below* Install cctools and clang-3.4 (don't build the sample project, which is not required and builds using libc++):
```
sudo port clean llvm-3.4
sudo port patch llvm-3.4
sudo rm -r /opt/local/var/macports/build/*/llvm-3.4/work/llvm-3.4.2.src/projects/sample
sudo port install llvm-3.4
sudo port install cctools +llvm34
sudo port install clang-3.4
sudo port select --set clang mp-clang-3.4 (so that clang and clang++ point to that version)
```

* Install gcc-4.9 and the Apple GCC driver (for building plugins with OpenMP support):
```
sudo port deactivate libunwind-headers 
sudo port install libgcc
sudo port install gcc49 +universal
git clone https://github.com/devernay/macportsGCCfixup.git
cd macportsGCCfixup
mkdir build
cd build
cmake ..
make
sudo make install
```

* Use a clean `PATH` for the rest of the installation (especially if you also have [Homebrew](http://brew.sh/) in `/usr/local`or [Fink](http://www.finkproject.org/) in `/opt/fink`):
```
PATH=/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin
```

* Install the following packages:
```
sudo port -v install qt4-mac boost glew cairo expat jpeg openexr ffmpeg openjpeg15 freetype lcms ImageMagick lcms2 libraw opencolorio openimageio flex bison openexr seexpr fontconfig py27-shiboken py27-pyside
```

##  Building Natron

* Get the build scripts from the Natron github repository:
```
git clone https://github.com/MrKepzie/Natron
```

* To build a snapshot:
```
cd Natron/tools/MacOSX
env MKJOBS=4 ./snapshot.sh
```

* To do a single build, modify TAGS in the begining of common.sh and then call the following:
```
cd Natron/tools/MacOSX
env CONFIG=relwithdebinfo BRANCH=tag  MKJOBS=4 UPLOAD=1 ./build.sh
```

The server will now auto build from the workshop branch on changes.

## Online repository

When building Natron you can upload it to a server (`UPLOAD=1`). For this to work you need to create a file named `local.sh` next to `snapshot.sh`, with for example the following content:
```
REPO_DEST=user@host:/path
REPO_URL=http://some.url
```

By default the snapshot.sh script will always upload hence you need to create the local.sh script to have snapshots working.
