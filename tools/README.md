# Contents of the `tools` Directory

This directory contains various tools to help building and maintaining Natron.

- **MacPorts**
  
  Repository for Natron-specific ports for MacPorts. Some ports are not in MacPorts (e.g. openimageio), some have extra patches that fix specific issues (licensing, universal builds, etc.). Next to each `Portfile`, we always store the original MacPorts `Portfile.orig` and the `Portfile.patch` to apply. The `check.sh` script helps checking which `Portfile`s have to be updated after a `sudo port selfupdate`.

- **genStaticDocs.sh**

  This is the script used to generate the HTML documentation which is shippend with Natron binaries. This script is executed by the buildmaster and jenkins scripts, but it can also be used to preview the result while writing documentation.
  
  The script requires [pandoc](https://pandoc.org/) and [Sphinx](http://sphinx-doc.org/).
  
  For example, on macOS, if a Natron snapshot or release is installed as `/Applications/Natron.app`, The static docs can be generated in the `html` subdirectory with the following commands:
  ```
  cd <Natron source directory>/Documentation
  natron="/Applications/Natron.app"
  env FONTCONFIG_FILE="$natron/Contents/Resources/etc/fonts/fonts.conf" \
  OCIO="$natron/Contents/Resources/OpenColorIO-Configs/blender/config.ocio" \
  OFX_PLUGIN_PATH="$natron/Contents/Plugins" \
  NATRON_PLUGIN_PATH="$natron/Contents/Plugins/PyPlugs" \
  bash ../tools/genStaticDocs.sh "$natron/Contents/MacOS/NatronRenderer" /var/tmp/natrondocs .
  sphinx-build -b html source html
  ```

- **jenkins**

  These scripts are used to:
  
  - build the Natron SDK on Linux 32bit and Linux64bit (any distribution):
	
    `./include/scripts/build-Linux-sdk.sh` builds and install all Natron dependencies to `/opt/Natron-sdk`, which must be a user-writable directory. This includes a patched Qt4, a [mangled OSMesa](https://github.com/devernay/osmesa-install) (with LLVM 4.0.1), OpenImageIO, and all other dependencies.

  - build the Natron SDK on Windows 7 with [MSYS2](https://www.msys2.org/) installed:
	
	  - `MSYSTEM=MINGW64 BITS=64 ./include/scripts/build-Windows-sdk.sh` builds and installs the 64bit versions of the required packages.
	  - `MSYSTEM=MINGW32 BITS=32 ./include/scripts/build-Windows-sdk.sh` builds and installs the 32bit versions of the required packages.
	  
	  Note that the system should also be regularly updated. Updates usually go this way:
	  
      - pull updates from your local clone of the official [MINGW-packages](https://github.com/Alexpux/MINGW-packages) repository.
	  - go to the `tools/MINGW-packages` directory in the Natron sources (detailed below), and execute `./check.sh` to see if any package needs updating. Update `PKGCONFIG` for these packages accordingly.
	  - execute `pacman -Syu` to update MSYS2

	    Answer "n" to those two questions, if `pacman -Syu` asks:

	    ```
		:: Replace mingw-w64-i686-x264 with mingw32/mingw-w64-i686-x264-git? [Y/n] n
	    :: Replace mingw-w64-x86_64-x264 with mingw64/mingw-w64-x86_64-x264-git? [Y/n] n
		``

	    After execution, `pacman -Syu` may ask you to close and relaunch the terminal.
	  
	  - execute `MSYSTEM=MINGW64 BITS=64 ./include/scripts/build-Windows-sdk.sh` to rebuild and install the Natron-specific packages.
	  
  - build the Natron binaries and execute the unit tests on OS X 10.9 (`macStartupJenkins.sh`), Linux CentOS 6.4 32bit and 64bit (`linuxStartupJenkins.sh`) and Windows 10 32bit and 64bit (`winStartupJenkins.bat`). These build scripts are basically updated versions of the scripts from the `buildmaster`directory, but were never tested to produce actual releases. Also note that the OS X script does not produce universal 32/64 bits binaries when run on OS X 10.9.

- **buildmaster**

  The directory, given for reference, contains the old scripts that were used to build Natron binaries on the Natron build farm. These scripts require a complicated infrastructure (build farm with one machine of each arch, database to store the build results, web server to store the binaries and build symbols, etc.).
  
  These scripts are here mainly for reference, and the jenkins scripts are probably more up-to-date, although they were never used to produce releases.
  
  The main script is `buildmaster.sh`, and we launch it that way:
  - on OS X 10.6, Linux CentOS 6.4 32bit, Linux CentOS 64bit: `env NATRON_LICENSE=GPL ./buildmaster.sh`
  - on Window 7 64bit, we run in parallel the scripts to build the 32bit and the 64bit versions:
    - `env NATRON_LICENSE=GPL MSYSTEM=MINGW32 BITS=32 MKJOBS=2  ./buildmaster.sh`
    - `env NATRON_LICENSE=GPL MSYSTEM=MINGW64 BITS=64 MKJOBS=2  ./buildmaster.sh`
  The script that builds the Windows packages is in `buildmaster/include/scripts/build-Windows-installer.sh`, and must be updated every type a dependency DLL filename changes (this was made automatic in the corresponding jenkins script).
  

- **docker**

  Contains tools to build a docker image that contains the Natron SDK installed on top of CentOS6.

  The `natron-sdk` docker image is also available from dockerhub, see https://hub.docker.com/r/natrongithub/natron-sdk
  
- **MINGW-packages**

  This directory contains Natron-specific packages for MSYS2. For example, therer are ffmpeg and libraw versions with different licences, a patched version of Qt4, and a version of OpenImageIO 1.7 (because later versions hang when quitting Natron, see [this OpenImageIO issue](https://github.com/OpenImageIO/oiio/issues/1795)).
  
  The `check.sh` script may be used to check whether packages need to be updated, supposing that `../../../MINGW-packages` contains a clone of [MINGW-packages](https://github.com/Alexpux/MINGW-packages)  (note that the `mingw-w64-pyside-qt4` and `mingw-w64-shiboken-qt4` have not been updated for a long time, because they just work).

- **license**

  This directory contains license files for Natron and all its dependencies (including the plugins dependencies). The main Natron distribution is GPL2, so please make sure that no dependencies is GPL3. Natron could be distributed with another licence (LGPL or commercial), if the code owners allow it, but in this case GPL2 components can not be used (for example, only LGPL versions of ffmpeg and libraw may be used, and poppler can not be used).
  
  These files are included by `Gui/GuiResources.qrc` in the Natron sources, and are available through the About window in Natron.
  
- **mkTarballs.sh**

  A script to make tarballs of Natron and plugins sources (probably needs to be updated or fixed).
  
- **normalize**

  The source for the `normalize` utility from Qt4, which normalizes the signatures in SIGNAL() and SLOT() macros through the Natron source code. This gives a very small speedup at execution. Most code-formatting tools usually break normalized signatures, so this tool should be run afterwards.
  
- **travis**

  Scripts used for [Travis CI builds of Natron](https://travis-ci.org/NatronGitHub/Natron). See also the hidden file `.travis.yml` at the top source directory.

- **uncrustify**

  Configuration file for [uncrustify](http://uncrustify.sourceforge.net/). Unfortunately, uncrustify also breaks things, so the source code needs to be fixed after running it:
  - run uncrustify first
  
    `uncrustify -c tools/uncrustify/uncrustify.cfg --replace */*.cpp */*.h`

  - fix the output of uncrustify using the provided sed script

    `sed -f tools/uncrustify/uncrustify-post.sed -i .bak */*.cpp */*.h`

  - run Qt's normalize utility to fix signals/slots
  
    `for d in Gui Engine App Global Renderer Tests; do tools/normalize/normalize --modify $d; done`

- **utils**

  A few more maintenance utilities:
  
  - `fixpngs.sh` removes any date reference from the PNGs in the source tree, and also runs [optipng](http://optipng.sourceforge.net/)
  
  - `generate_glad.sh` generates the glad source files for properly handling the OpenGL extensions.
  
  - `runPostShiboken.sh` is used to fix source files after running [shiboken](https://github.com/pyside/Shiboken) to generate the Python bindings. See the section *Generating Python bindings* in `INSTALL_OSX.md` and `INSTALL_LINUX.md`.
valgrind

- **valgrind**

  [valgrind](http://valgrind.org/) suppression files.
  
  There is also a suppression file for Python in `jenkins/include/natron/valgrind-python.supp`and `buildmaster/include/natron/valgrind-python.supp`
