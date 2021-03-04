Natron Continuous Integration Tools
===================================

Scripts used to build and distribute [Natron](http://www.natron.fr) on Linux, Windows and Mac.

Linux Setup
===========

 * Download http://mirror.nsc.liu.se/centos-store/6.4/isos/x86_64/CentOS-6.4-x86_64-minimal.iso (or http://mirror.nsc.liu.se/centos-store/6.4/isos/i386/CentOS-6.4-i386-minimal.iso)
 * Install ISO (remember to setup network)
 * Boot server
 * If on jenkins, the system drive is maybe not big enough to fit all build files, an additional drive may be installed using the following procedure (as root):  https://www.ubiquityhosting.com/blog/partition-format-and-mount-a-secondary-drive-in-linux/
    
 * Add user "ci" and set its password to "ci" and add it to the sudoers
 * Extract the gnupg.tar.gz archive in ci's home directory (used for signing the RPMs)
 * Enable network:
   - `sudo vi /etc/sysconfig/network-scripts/ifcfg-eth0`
   - Replace ONBOOT=no by ONBOOT=yes , then save and exit file.
   - Restart network service: `sudo service network restart`
 * Now that network is enabled, you can clone the natron-support repository: See Cloning natron-support section below.
 * Run buildmaster/include/scripts/setup-centos6.sh
 * Update git to a version > 2, see https://stackoverflow.com/questions/21820715/how-to-install-latest-version-of-git-on-centos-6-x-7-x/38133865#38133865 :
```
sudo yum remove git
sudo yum -y install epel-release perl-devel
wget https://centos6.iuscommunity.org/ius-release.rpm
sudo yum install ius-release-1.0-15.ius.centos6.noarch.rpm
```
If 32 bit linux, install:
```
sudo yum -y install ftp://mirror.switch.ch/pool/4/mirror/scientificlinux/6.4/i386/updates/security/openssl-1.0.1e-16.el6_5.15.i686.rpm
```
Otherwise if 64 bit:
```
sudo yum -y install ftp://mirror.switch.ch/pool/4/mirror/scientificlinux/6.4/x86_64/updates/security/openssl-1.0.1e-16.el6_5.15.x86_64.rpm
```
Then:
```
sudo yum -y install git2u
```
 * Install tmux: `sudo yum -y install libevent tmux`
 * Launch tmux: `tmux`
 * Build SDK: run `include/scripts/build-Linux-sdk.sh` (this will take forever, it's recommended to use a prebuilt archive)
 * Detach tmux while building and reconnect when done:
    type ctrl+b then type 'd' in tmux to detach., to reattach: `tmux attach`

Windows Setup
=============

 * Install Windows 7 Pro/Enterprise 64bit

Note that if this is a VM created on the ci-inria, you must install update the storage controller driver with this iso:

https://fedoraproject.org/wiki/Windows_Virtio_Drivers#Direct_download

You can mount the iso using daemon tools.
Then launch Administrative Tools.exe and in Computer Management select Devices Manager, expand Storage controllers and right click "Update driver" and select the location where the iso was mounted.
The disk should be available now from the Storage/Disk Management area of Computer Management (still in Administrative Tools.exe). Right click and add a volume on the disk with NTFS formatting.

 * Install http://repo.msys2.org/distrib/msys2-x86_64-latest.exe (or http://repo.msys2.org/distrib/msys2-i686-latest.exe if on a 32-bit machine) from http://www.msys2.org/ . The default install location is `C:\msys64` (`C:\msys32` on a 
 * Start MSYS terminal: Start, All Programs, MSYS2 64bit (or 32bit), MSYS2 MSYS
 * run `pacman -Syu` to update all packages and the locally cached repository information. You may need to close the terminal, reopen it, and relaunch `pacman -Syu`: just follow the directions.
 * run `pacman -S openssh git tar bsdtar cygrunsrv libevent tmux nano` to install basic admin tools.
 * get and install the buildmaster ssh key. It can be obtained from the Natron-support [source tree browser](https://gforge.inria.fr/scm/browser.php?group_id=7940) after logging in to http://gforge.inria.fr (click on "tree" above the shortlog, then on `buildmaster`, then on `id_rsa_buildmaster.tar.gz`.
 * Clone the `natron-support`: See Cloning natron-support section below.
 * `git checkout jenkins` if on jenkins, or checkout the jenkins version if on evaporation (the physical Windows machine) - the jenkins branch is used to build the SDK: `git clone -b jenkins git+ssh://natron-ci@scm.gforge.inria.fr/gitroot/natron-support/natron-support.git natron-support-jenkins`
 * Close the MSYS terminal.
 * Start MSYS terminal with admin rights: Start, All Programs, MSYS2 64bit (or 32bit), right click on MSYS2 MSYS, Run as administrator
 * Install ssh-server using the script located in `buildmaster/include/scripts/msys2-install-sshd.sh` (only available in the `jenkins` branch, and inspired by [this gist](https://gist.github.com/samhocevar/00eec26d9e9988d080ac)). It may tell you to run `pacman -S mingw-w64-x86_64-editrights` before relaunching the script.
 * Ensure the windows firewall accepts ports 22 for sshd:
```
netsh advfirewall firewall add rule name="sshd" dir=in action=allow protocol=TCP localport=22
netsh advfirewall firewall add rule name="sshd" dir=out action=allow protocol=TCP localport=22
```
 * The best way to log into the machine is to put use buildmaster key as an authorized_key: `cp .ssh/id_rsa_build_master .ssh/authorized_keys`
 * To avoid a warning when logging in with ssh: `touch /var/log/lastlog`
 * if a command is missing, try `pacman -Fyxs <command>`
 * Build SDK: launch `tmux`, `cd buildmaster`, and run `env NATRON_LICENSE=GPL BITS=64 MKJOBS=2 include/scripts/build-Windows-sdk.sh`
 * On 64+32 systems, the script has to be run twice (first with `MSYSTEM=MINGW64`, then with `MSYSTEM=MINGW32` and `PATH=/mingw32/bin:$PATH`) to build llvm+osmesa and imagemagick6+7 for both archs. This should be fixed (TODO).
 * The old `buildmaster.sh`script which runs on the physical machine `evaporation` also requires a MariaDB client. Get and install [mariadb-latest-x64.msi](https://downloads.mariadb.org/f/mariadb-10.2.9/winx64-packages/mariadb-10.2.9-winx64.msi) (only the ClientPrograms are necessary), then `ln -s "/c/Program Files/MariaDB 10.2" /opt/mariadb`. you also need to create `settings.sh` in the buildmaster directory: `echo SERVERNAME=evaporation > settings.sh`
 * If the command `git submodule update` outputs an error that looks like `': not a valid identifierline 88: export: 'dashless` (see [this MSYS2 issue](https://github.com/Alexpux/MSYS2-packages/issues/735)), add the following to `~/.bashrc`: `alias git="PATH=/usr/bin git"`
 * Local packages are described in the directory `msys2-packages`
 * To restart from scratch before reinstalling the SDK, simply `pacman -Rsc $(pacman -Q |fgrep mingw|awk '{print $1}')` and accept to remove all dependencies. You can then update MSYS with `pacman -Syu`.

Mac Setup
=========

If homebrew is installed (as on the defaults virtual machines on http://ci.inria.fr), it should be uninstalled using:

    brew list -1 | xargs brew rm

Disable the screensaver for the current host and user:

    defaults -currentHost write com.apple.screensaver idleTime 0
    defaults write com.apple.screensaver idleTime 0

The user must be logged in, or the disk image creation at the end of the build will fail. Make sure that automatic login is set up, as described in [HT201476](https://support.apple.com/HT201476).

See the `INSTALL_MACOS.md` file in the main Natron repository. It contains all installation steps as well as setup of macports. 


Cloning natron-support
======================

A specific 'natron-ci' user has been created on the repository whose role is to build and distribute Natron.
You should not need to create a new user. 
A SSH private/public key pair is already created that grants access to the repository for this user.
The key is located in the tarball archive in `buildmaster/id_rsa_buildmaster.tar.gz`.
It can be obtained from the Natron-support [source tree browser](https://gforge.inria.fr/scm/browser.php?group_id=7940) after logging in to http://gforge.inria.fr (click on "tree" above the shortlog, then on `buildmaster`, then on `id_rsa_buildmaster.tar.gz`, or by using scp if the ssh client is installed:
```
scp <...>/natron-support/buildmaster/id_rsa_buildmaster.tar.gz ci@<ipoflinuxbox>:.
```
On the build machine:
```
tar xvf id_rsa_build_master.tgz
mkdir ~/.ssh
mv id_rsa_buildmaster* ~/.ssh
```
Ensure your ssh-agent is running:
```
eval $(ssh-agent)
```
Add the ssh-key:
```
ssh-add ~/.ssh/id_rsa_buildmaster
```
Clone (note: `@`is `Alt-Gr-à` on the French Windows keyboard):
```
git clone git+ssh://natron-ci@scm.gforge.inria.fr/gitroot/natron-support/natron-support.git
```
Update submodules:
```
cd natron-support
git submodule update -i --recursive
```
The password on the gforge of the natron-ci user is: Natron64! (with the exclamation mark)

To automatically enable the ssh key, using [keychain](https://www.funtoo.org/Keychain):
```
cd
wget https://build.funtoo.org/distfiles/keychain/keychain-2.8.4.tar.bz2
tar xvf keychain-2.8.4.tar.bz2
sudo cp keychain-2.8.4/keychain /usr/local/bin/keychain
ssh-keygen -y -f ~/.ssh/id_rsa_build_master > ~/.ssh/id_rsa_build_master.pub
echo 'eval `keychain --eval --agents ssh id_rsa_build_master`' >> .bash_profile
```

Now logout and login, and you must connect to downloads.natron.fr (do it at least once to get the host signature and approve it):
```
ssh buildmaster@downloads.natron.fr
exit
```

Launching a build
=================

The old buildmaster.sh script is deprecated and only works with the workstation at INRIA and the natron.fr/builds frontend.
A new version was ported to launchBuildMain.sh which does not require any external database and may be called directly with options from the command line and is designed to work with the jenkins CI infratstructure. 

Example launch command:
```
NATRON_LICENSE=GPL GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=master GIT_COMMIT=f7ee2a2c43bd0de9fc8d3776346408dd4f43426b UNIT_TESTS=true BUILD_NUMBER=1 BUILD_NAME=test COMPILE_TYPE=relwithdebinfo MKJOBS=4 ./launchBuildMain.sh
```

Options
-------

The following options can be passed to `launchBuildMain.sh` as environment variables:

`WORKSPACE`: The absolute path to a directory on the local filesystem were builds are done, external files are downloaded, and builds are archived.

`RELEASE_TAG`: Make a release build. It should be a string "x.y.z" indicating the release number. The corresponding tag is "vx.y.z" in the Natron repository and "Natron-x.y.z" in each of the plugins repository (openfx-io, openfx-misc, openfx-arena, openfx-gmic). Only indicate this to trigger a release.

`SNAPSHOT_BRANCH`: If set and `RELEASE_TAG` is not set, this indicates the branch on which to launch a snapshot from. If `SNAPSHOT_COMMIT` is also set, this will build this specific commit on that branch, otherwise it will build the latest commit on the branch. If `GIT_COMMIT` is set, `GIT_BRANCH` is ignored.

`SNAPSHOT_COMMIT`: If set, this indicate the commit on which to launch a snapshot from. This has to be set in combination with `SNAPSHOT_BRANCH`. If `GIT_COMMIT` is set, `GIT_BRANCH` is ignored.

`UNIT_TESTS`: Run unit tests after build. Value can be "false" or "true" (for compatibility with jenkins).

`NATRON_LICENSE`: Must be `GPL` or `COMMERCIAL`. When GPL is selected, GPL components (such as certain codecs of ffmpeg or demosaic patterns of libraw) will be included.

`DISABLE_BREAKPAD`: If set to 1, natron will be built without crash reporting capabilities.

`COMPILE_TYPE`: The type of build to do, i.e in terms of compilation. Valid values are (`relwithdebinfo`, `release`, `debug`). Must be `relwithdebinfo` or `debug` if DISABLE_BREAKPAD is not 1.

`BITS`: Windows only: Must indicate if this is a 64 or 32 bits build.

`NATRON_DEV_STATUS`: `ALPHA`, `BETA`, `RC`, `STABLE` or `CUSTOM`. This is only useful when doing a release, i.e. if specifying `RELEASE_TAG`.

`NATRON_CUSTOM_BUILD_USER_NAME`: When `NATRON_DEV_STATUS` is `CUSTOM`, tag the build with a specific user/client name that will be displayed within Natron.

`NATRON_BUILD_NUMBER`: When doing a release this is the number of the release (if doing a rebuild).

`NATRON_EXTRA_QMAKE_FLAGS`: Optional qmake flags to pass when building Natron.

`BUILD_NAME`: Set this to label the project build. On the slave, the build artifacts are stored for some amount of time in `$WORKSPACE/builds_archive/$BUILD_NAME/$BUILD_NUMBER`.

`BUILD_NUMBER`: A unique number to identify the build (usually incremented at each build). See also `BUILD_NAME`.

`DISABLE_RPM_DEB_PKGS`: If set to 1, deb and rpm packages will never be built. Default is to build when `NATRON_BUILD_CONFIG`=`STABLE`.

`DISABLE_PORTABLE_ARCHIVE`: If set to 1, a portable archive will not be built (Windows and Linux).

`REMOTE_URL`: The URL that the online installer will use to check for updates. 
This should be consistent with the `REMOTE_URL` value passed to the upload-artifacts project.

`REMOTE_USER`: The name of the user on `REMOTE_URL` that should be used when uploading using rsync.
This should be consistent with the `REMOTE_USER` value passed to the upload-artifacts project.

`REMOTE_PREFIX`: The prefix to upload artifacts to, relative to `REMOTE_USER@REMOTE_URL`'s home dir.
This should be consistent with the `REMOTE_PREFIX_USER` value passed to the upload-artifacts project.

`WITH_ONLINE_INSTALLER`: Set to 1 to also build a online installer for `SNAPSHOT` and `RELEASE` builds (only available for Windows and Linux).

`DEBUG_SCRIPTS`: If set to 1, binaries from previous build with same options are not cleaned so that the scripts can continue the same compilation.

`EXTRA_PYTHON_MODULES_SCRIPT`: Path to a python script that should install extra modules with `pip` or `easy_install`.


Upgrading a library on the build machine
=========================================

Linux
-----

The `include/scripts/build-Linux-sdk.sh` script has to be launched. Libraries that already exist with the same version in the `SDK_HOME` will not be built. To decide whether a library should be built or not, the script usually simply tests that a file from the package exists at the beginning of the build section. Top force a rebuild, check in the script to see which file is tested, remove it, and launch the script.

To upgrade a library version, upload the source files to `downloads.natron.fr/Third_Party_Sources`. This is where the scripts has been configured to fetch source files from (but it may also get it from the main site for the given package - this location is a failsafe place). To change this location edit common.sh.
Then in `common.sh`, change the filename of the package to point to the file that was uploaded on the server.

To include a new patch, add it in `include/patches/LIBRARY/patch`. Patch and edit the sdk script to run the patch when needed.

To produce a tarball of the SDK on the build-machine (so it can be re-used somewhere), use the `TAR_SDK=1` option before calling the sdk script;
To also upload the sdk to `downloads.natron.fr/Third_Party_Binaries` you may also provide the option `UPLOAD_SDK=1`.

Windows
-------

Same as linux, except that the script is `include/scripts/build-Windows-sdk.sh`


Mac:
----

See `INSTALL_MACOS.md` in the main Natron repository: libraries are handled by macports. To update a Natron-specific package, the MacPorts `Portfile` located in `Natron/tools/MacPorts` can to be updated.


Jenkins
-------

Launching a build manually on the VM on ci-inria:
Set `DEBUG_SCRIPTS` to 1 to keep all elements built and path checked out so that if re-triggering a jenkins.sh
call attempts to re-use an existing build.
```
export DEBUG_SCRIPTS=1 
JOB_NAME="CustomBuild" BUILD_NUMBER=1 GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=master GIT_COMMIT=08f891ceb8b21ce83eb224f8e035d700310580d2 UNIT_TESTS=true NATRON_LICENSE=GPL ./jenkins.sh
```







