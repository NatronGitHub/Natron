Natron on MacOSX
================

Scripts used to build and distribute [Natron](http://www.natron.fr) on MacOSX.


Build server installation
=========================

 * Download and install MacOSX Snow Leopard 10.6 retail DVD.

 * Install [MacPorts](https://www.macports.org/install.php)

 * Install Xcode 3.2.6 for Snow Leopard from developer.apple.com. Don't forget to also install the UNIX command-line utilities (this is an install option in Xcode 3).

 * Download Macports [dports-dev](http://downloads.natron.fr/Third_Party_Sources/dports-dev.zip)

 * Give read/execute permissions to the local repository: "chmod 755 /Users/USER_NAME/Development/dports-dev"

 * Check that the user "nobody" can read this directory by typing the following command in a Terminal: "sudo -u nobody ls /Users/USER_NAME/Development/dports-dev". If it fails, then try again after having given execution permissions on your home directory using the following command: "chmod o+x /Users/USER_NAME". If this still fails, then something is really wrong.

 * Edit the sources.conf file for MacPorts, for example using the nano editor: "sudo nano /opt/local/etc/macports/sources.conf", insert at the beginning of the file the configuration for a local repository (read the comments in the file), by inserting the line "file:///Users/USER_NAME/Development/dports-dev" (without quotes, and yes there are 3 slashes). Save and exit (if you're using nano, this means typing ctrl-X, Y and return).

 * Update MacPorts: "sudo port selfupdate"

 * Recreate the index in the local repository: "cd /Users/USER_NAME/Development/dports-dev; portindex" (no need to be root for this)

 * Add the following line to /opt/local/etc/macports/variants.conf:
    -x11 +no_x11 +bash_completion +no_gnome +quartz +universal

 * Install clang-3.4:

    sudo port install clang-3.4
    sudo port select --set clang mp-clang-3.4 (so that clang and clang++ point to that version)

 * Install gcc-4.8 and the Apple GCC driver:

    sudo port install gcc48 +universal
    git clone https://github.com/devernay/macportsGCCfixup.git
    cd macportsGCCfixup
    ./configure
    make
    sudo make install

 * Install the following packages:

    sudo port install qt4-mac boost glew cairo expat jpeg openexr ffmpeg openjpeg15 freetype lcms ImageMagick lcms2 libraw opencolorio openimageio flex bison openexr seexpr fontconfig py27-shiboken py27-pyside


 * git clone https://github.com/MrKepzie/Natron 
 
 * To enable snapshots:
    
    env MKJOBS=4 Natron/tools/MacOSX/snapshot.sh

  * To do a single build, modify TAGS in the begining of common.sh and then call the following:
    
    env CONFIG=relwithdebinfo BRANCH=tag  MKJOBS=4 UPLOAD=1 Natron/tools/MacOSX/build.sh
    

The server will now auto build from the workshop branch on changes.

Online repository
==================

When building Natron you can upload it to a server (UPLOAD=1). For this to work you need to create a file named **local.sh** next to *snapshot.sh*, with for example the following content:

    #!/bin/bash

    REPO_DEST=user@host:/path
    REPO_URL=http://some.url

By default the snapshot.sh script will always upload hence you need to create the local.sh script to have snapshots working.
