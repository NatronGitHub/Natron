Natron Build Master
===================

Scripts used to build and distribute [Natron](http://www.natron.fr) on Linux, Windows and Mac.

Linux Setup
===========

 * Download http://mirror.nsc.liu.se/centos-store/6.4/isos/x86_64/CentOS-6.4-x86_64-minimal.iso (or http://mirror.nsc.liu.se/centos-store/6.4/isos/i386/CentOS-6.4-i386-minimal.iso)
 * Install ISO (remember to setup network)
 * Boot server
 * Update git to a version > 2, see https://stackoverflow.com/questions/21820715/how-to-install-latest-version-of-git-on-centos-6-x-7-x/38133865#38133865
 * Copy 'include/scripts/setup-centos6.sh' to server using SSH
 * login to build server as root
 * run 'setup-centos6.sh' (this may take a while)
 * Clone the natron-support: See Cloning natron-support section below.
 * Build SDK: run 'include/scripts/build-Linux-sdk.sh' (this will take forever, it's recommended to use a prebuilt archive)

Windows Setup
=============

 * Install Windows 7 Pro/Enterprise 64bit
 * Install MSYS2 32+64 bit
 * Start mingw terminal (NOTE! you must use the correct terminal according to the BIT you want to build for)
 * Clone the natron-support: See Cloning natron-support section below.
 * git clone https://github.com/MrKepzie/MINGW-packages in this folder
 * run 'include/scripts/setup-msys.sh'
 * Build SDK: run 'include/scripts/build-Windows-sdk.sh' (may take a while) NOTE! as MSYS2 is a moving target doing a clean install will most likely break something/everything in the installer script.

Mac Setup
=========

See the INSTALL_OSX.md file in the main Natron repository. It contains all installation steps as well as setup of macports. 


Cloning natron-support
======================

A specific 'natron-ci' user has been created on the repository whose role is to build and distribute Natron.
You should not need to create a new user. 
A SSH private/public key pair is already created that grants access to the repository for this user.
The key is located in the tar.gz archive in buildmaster/buildmaster_ssh_key.tar.gz. 
You just need to scp it from the build machine to the machine your are reading this from.

On the build machine:

    scp USERNAME@remoteComputer:natron-support/buildmaster/buildmaster_ssh_keys.tar.gz .

    tar -xvf buildmaster/buildmaster_ssh_keys.tar.gz

Ensure your ssh-agent is running:

    eval $(ssh-agent)

Add the ssh-key:

    ssh-add buildmaster/buildmaster_ssh_keys/id_rsa

Clone:

    git clone https://natron-ci@scm.gforge.inria.fr/authscm/natron-ci/git/natron-support/natron-support.git

Update sumbodules:

    git submodule update -i --recursive

The password on the gforge of the natron-ci user is: Natron64! (with the exclamation mark)

To automatically enable the ssh key, you can add to your .bashrc the following lines:

    if [ ! -S ~/.ssh/ssh_auth_sock ]; then
        eval $(ssh-agent)
        ln -sf "$SSH_AUTH_SOCK" ~/.ssh/ssh_auth_sock
    fi
    export SSH_AUTH_SOCK=~/.ssh/ssh_auth_sock
    ssh-add ~/id_rsa_build_master


Online repository and database
===============================

The buildmaster script works with a mysql database to fetch the build queue (and synchronize all instances of buildmaster accross build machines).
It also needs to upload the logs and the resulting files to the webserver.

All the server dependent stuff is setup in variables in common.sh. Make sure that everything is setup as needed.

Launching buildmaster
=====================

Export the wanted variables, like MKJOBS, BITS, NATRON_LICENSE.

    ./buildmaster.sh

Do **not** use env ..... ./buildmaster.sh or sh buildmaster.sh. Why? buildmaster.sh relaunches when needed and variables etc will get lost.

**NOTE:** If you run mingw32+mingw64 on the same machine you will need to define seperate TMP's, I usually export TMP=/tmp64 for mingw64, this is important since the installer for each BITS uses TMP and you will get conflicts (and failed builds).

Snapshots and releases are now controlled from https://natron.fr/builds

Upgrading a library on the build machine
=========================================

Linux
-----

The `include/scripts/build-Linux-sdk.sh` script has to be launched. Libraries that already exist in the `SDK_HOME` will not be built unless specified with a `REBUILD_XXX=1` variable before running the script.

To upgrade a library version, upload the source files to `downloads.natron.fr/Third_Party_Sources`. This is where the scripts has been configured to fetch source files from. To change this location edit `common.sh`.
Then in `common.sh`, change the filename of the package to point to the file that was uploaded on the server.

To include a new patch, add it in `include/patches/LIBRARY/patch`. Patch and edit the sdk script to run the patch when needed.

To produce a tarball of the SDK on the build-machine (so it can be re-used somewhere), use the `TAR_SDK=1` option before calling the sdk script;
To also upload the sdk to `downloads.natron.fr/Third_Party_Binaries` you may also provide the option `UPLOAD_SDK=1`.

Windows
-------

Same as linux, except that the script is `build-Windows-sdk.sh`


Mac:
----

See `INSTALL_OSX.md` in the main Natron repository: libraries are handled by macports. To change a library version, the portfile of macports located in `Natron/tools/macports` in the **master** branch have to be updated.



Connecting to existing servers
===============================


- Remote login to the linux 'natron' (Dell T7600 Precision) buildstation located at INRIA:
Username is 'user', password is 'ubuntu16'
It is running 2 CentOS VM (32 and 64bit)
<MISSING DOC>

- Remote login to the osx 'mistral' (MacPro 2012) buildstation located at INRIA:
Username is 'user', passwoard is 'osx106'

- Remote login to the Windows 7 workstation:

In your VLC client:
```
IP: 194.199.26.228:
Port: 5900
User/passwd: NatronWinBuild

gforge natron-ci account (used by buildmaster to build):

login: natron-ci
pwd: NatronWinBuild2017!
```










