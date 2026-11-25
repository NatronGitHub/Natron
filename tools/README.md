# Contents of the `tools` Directory

This directory contains various tools to help building and maintaining Natron.

- **MacPorts**
  
  Repository for Natron-specific ports for MacPorts. Some ports are not in MacPorts (e.g. openimageio), some have extra patches that fix specific issues (licensing, universal builds, etc.). Next to each `Portfile`, we always store the original MacPorts `Portfile.orig` and the `Portfile.patch` to apply. The `check.sh` script helps checking which `Portfile`s have to be updated after a `sudo port selfupdate`.

- **genStaticDocs.sh**

  This is the script used to generate the HTML documentation which is shippend with Natron binaries. This script is executed by the jenkins scripts, but it can also be used to preview the result while writing documentation.
  
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

  The release build scripts: `launchBuildMain.sh` checks out, builds and packages Natron and the plug-ins (the macOS DMG, the Linux portable archive, the Windows portable zip with Inno Setup's `Setup.exe`), and runs the unit tests. See [jenkins/README.md](jenkins/README.md).

  `./include/scripts/build-Linux-sdk.sh` builds and installs all the Natron dependencies of the Linux SDK to `/opt/Natron-sdk`, which must be a user-writable directory. It is normally run inside the Rocky Linux 9 Docker image (see **docker** below).

  On Windows, the SDK is MSYS2's UCRT64 environment with the packages of **MINGW-packages** (see below).

- **docker**

  Contains the tools to build the Rocky Linux Docker image with the Natron Linux SDK (`natron-sdk-rockylinux`). A build is launched with `docker run` on that image. See [docker/README.md](docker/README.md).

- **tart**

  Builds Natron for macOS in a clean macOS 14 virtual machine on an Apple Silicon Mac. See [tart/README.md](tart/README.md).

- **MINGW-packages**

  Natron's own MSYS2 packages, built in the UCRT64 environment, for the plug-ins and the Windows installer: only what MSYS2 doesn't provide in the form Natron needs (FFmpeg built GPLv2, LibRaw with and without the GPL demosaic packs, OpenImageIO built against that LibRaw, SeExpr, a static ImageMagick 7, OSMesa, the Inno Setup installer and the build-deps meta-packages). See [MINGW-packages/README.md](MINGW-packages/README.md) for the list, and for how to rebuild and publish the package repository.

  The `check.sh` script reports the packages whose upstream PKGBUILD changed, supposing that `../../../MINGW-packages` contains a clone of [msys2/MINGW-packages](https://github.com/msys2/MINGW-packages).

- **license**

  This directory contains license files for Natron and all its dependencies (including the plugins dependencies). The main Natron distribution is GPL2, so please make sure that no dependencies is GPL3. Natron could be distributed with another licence (LGPL or commercial), if the code owners allow it, but in this case GPL2 components can not be used (for example, only LGPL versions of ffmpeg and libraw may be used, and poppler can not be used).
  
  These files are included by `Gui/GuiResources.qrc` in the Natron sources, and are available through the About window in Natron.
  
- **mkTarballs.sh**

  A script to make tarballs of Natron and plugins sources (probably needs to be updated or fixed).
  
- **normalize**

  The source for the `normalize` utility from Qt4, which normalizes the signatures in SIGNAL() and SLOT() macros through the Natron source code. This gives a very small speedup at execution. Most code-formatting tools usually break normalized signatures, so this tool should be run afterwards.
  
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
  
  - `sourceCleanup.py` fixes the source files generated by Shiboken for the Python bindings, and `sourceList.py` lists them. The CMake build runs both; for the qmake build, see the section *Generating Python bindings* in `INSTALL_MACOS.md` and `INSTALL_LINUX.md`.

- **valgrind**

  [valgrind](http://valgrind.org/) suppression files.
  
  There is also a suppression file for Python in `jenkins/include/natron/valgrind-python3.supp`.
