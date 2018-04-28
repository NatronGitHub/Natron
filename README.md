Natron [![GPL2 License](http://img.shields.io/:license-gpl2-blue.svg?style=flat-square)](https://github.com/NatronGitHub/Natron/blob/master/LICENSE) [![Build Status](https://api.travis-ci.org/NatronGitHub/Natron.png?branch=master)](https://travis-ci.org/NatronGitHub/Natron)  [![Coverage Status](https://coveralls.io/repos/NatronGitHub/Natron/badge.png?branch=master)](https://coveralls.io/r/NatronGitHub/Natron?branch=master) [![Coverity Scan Build Status](https://scan.coverity.com/projects/15152/badge.svg)](https://scan.coverity.com/projects/natrongithub-natron "Coverity Badge") [![Documentation Status](https://readthedocs.org/projects/natron/badge/?version=rb-2.3)](http://natron.readthedocs.io/en/rb-2.3/) [![BountySource Status](https://api.bountysource.com/badge/team?team_id=271309)](https://www.bountysource.com/teams/natrongithub/issues?utm_source=Mozilla&utm_medium=shield&utm_campaign=bounties_received)
======


Natron is a free open-source (GPLv2 license) video compositing
software, similar in functionality to Adobe After Effects, Nuke by The
Foundry, or Blackmagic Fusion.

It is portable and cross-platform (GNU/Linux, OS X, Microsoft Windows).

The project home page is http://natron.fr

The project source code repository is https://github.com/NatronGitHub/Natron

Features
--------

- 32 bits floating point linear colour processing pipeline.
- Colorspace management handled by the OpenColorIO library.
- Dozens of file formats supported: EXR, DPX,TIFF, JPG, PNG…thanks to OpenImageIO and FFmpeg.
- Support for many free and open-source OpenFX plugins:
  * [TuttleOFX](https://sites.google.com/site/tuttleofx/)
  * [OpenFX-IO](https://github.com/NatronGitHub/openfx-io) to read anything else
   than standard 8-bits images
  * [OpenFX-Misc](https://github.com/NatronGitHub/openfx-misc)
  * [OpenFX-Vegas SDK samples](https://github.com/NatronGitHub/openfx-vegas)
  * [OpenFX samples](https://github.com/NatronGitHub/openfx) (in the Support and Examples directories)

- Support for commercial OpenFX plugins:
  * [All OFX products from RevisionFX](http://www.revisionfx.com)
  * [Furnace by The Foundry](http://www.thefoundry.co.uk/products/furnace/)
  * [KeyLight by The Foundry](http://www.thefoundry.co.uk/products/plugins/keylight/)
  * [GenArts Sapphire](http://www.genarts.com/software/sapphire/overview)
  * [Other GenArts products](http://www.genarts.com/software/other-vfx-products)
  * And many more. Please tell us if you successfully tested other commercial plugins.

- OpenFX: Currently almost all features of OpenFX v1.4 are supported

- Intuitive user interface: Natron aims not to break habits by providing an intuitive and familiar user
interface.  It is possible to customize and separate on any number of screens the graphical user interface.
You can re-use your layouts and share your layout files (.nl)

- Performances:  Never wait for anything to be rendered, in Natron anything you do produces
real-time feedback thanks to its optimised multi-threaded rendering pipeline and its support for proxy rendering (i.e:
the render pipeline can be computed at lower res to speed-up rendering).

- Multi-task: Natron can render multiple graphs at the same time, it can also be used
as a background process in command-line mode without any display support (e.g. for render farm purpose).

- Recover easily from bugs: Natron sometimes crashes. Fear not, an  auto-save system
detects inactivity and saves your work for yourself. Also Natron provides the option to render
a graph in a separate process, meaning that any crash in the main application
would not crash the ongoing render (and the other way around).

- Project format written in XML and easily editable by human.

- Fast & interactive Viewer - Smooth & accurate  zooming/panning even for very large image sizes (
tested on 27k x 30k images).

- Real-time playback: Natron offers  a real-time playback with best performances thanks to its
RAM/Disk cache technology. Once a frame is rendered, it can be reproduced instantly afterwards, even
for large image sizes.

- Low hardware requirements: All you need is an x86 64 bits or 32 bits processor, at least
3 GB of RAM and a graphic card that supports OpenGL 2.0 or OpenGL 1.5 with some extensions.

- Animate your visual effects: Natron offers a simple and efficient way to deal with keyframes
with a very accurate and intuitive Curve Editor as well as a Dope Sheet to quickly edit your motion graphics.

- Command line tool for execution of project files. The command line version is executable
 on a computer without any display. Hence it is possible to use a render farm
  to render Natron's projects.

- Rotoscoping, rotopainting and tracking support

- Multi-view workflow: Natron saves time by keeping all the views in the same stream. You can separate
the views at any time with the OneView node.

- Python 2 scripting integration:
    * Parameters expressions
    * User-defined parameters
    * Nodes groups as Python scripts
    * Script editor to control the application overall
    * User-defined python callbacks to respond to particular checkpoints of the internals of the software (change of a parameter, before rendering a frame, etc…)
    * Integration of Pyside to the GUI so that the interface is extensible with new menus and windows

- Multi-plane: Natron is able to deal with multi-layered EXR files thanks to OpenImageIO. It is deeply integrated into the workflow and the user can choose
to work with any layer (or plane) on any node. New custom layers can also be created.

Requirements
------------

A machine running one of the supported operating systems (GNU/Linux, OS X,
Microsoft Windows), and a 32-bits x86 or 64-bits x86-64 processor.

An OpenGL 2.0 compatible graphics card is needed to run Natron (2.1+) with hardware-accelerated rendering. Other graphics cards work with software-only rendering (see below).

The following graphics cards are supported for hardware-accelerated rendering:

* Intel GMA 3150 (Linux-only)
* Intel GMA X3xxx (Linux-only)
* Intel GMA X4xxx (Windows 7 & Linux)
* Intel HD (Ironlake) (Windows 7 & Linux)
* Intel HD 2000/3000 (Sandy Bridge) (Windows 7/Linux/Mac)
* Intel HD 4000 and greater (All platforms)
* Nvidia GeForce 6 series and greater
* Nvidia Quadro FX and greater
* Nvidia Quadro NVS 285 and greater
* ATI/AMD Radeon R300 and greater
* ATI/AMD FireGL T2-64 and greater (FirePro)

On Windows and Linux you can enable software rendering. On Linux, enable the environment variable LIBGL_ALWAYS_SOFTWARE=1 before running Natron. On Windows, enable the legacy hardware package in the installer.


Installing
----------

### Binary distribution ###

Standalone binary distributions of Natron are available for [GNU/Linux](http://downloads.natron.fr/Linux/),
[Windows](http://downloads.natron.fr/Windows/) and [OS X](http://downloads.natron.fr/Mac/). These distributions contain Natron and three basic sets of OpenFX plugins:
* [openfx-io](https://github.com/NatronGitHub/openfx-io/),
* [openfx-misc](https://github.com/NatronGitHub/openfx-misc),
* [openfx-arena](https://github.com/NatronGitHub/openfx-arena).

For each architecture / operating system, you can either download a stable release, a release candidate (if available), or one of the latest snapshots. Note that snapshots contain the latest features and bug fixes, but may be unstable.

### Building and installing from source ###

There are instructions for building Natron and the basic plugins from source is this directory on various architectures / operating systems:
* [GNU/Linux](INSTALL_LINUX.md)
* [OS X](INSTALL_OSX.md)
* [FreeBSD](INSTALL_FREEBSD.md)
* [Windows](INSTALL_WINDOWS.md)

This documentation may be slightly outdated, so do not hesitate to submit updated build instructions, especially for the various GNU/Linux distributions.

### Automatic build scripts ###

There are automatic build scripts for [GNU/Linux](tools/linux), [OS X](tools/MacOSX), and [Windows](tools/Windows), which are used to build the binary distributions and the snapshots.

These scripts are run on virtual machines running a specific version of each operating system, and setting these up is more complicated than for the basic builds described above.

There is some documentation, which is probably outdated, for [GNU/Linux](tools/linux/README.md), [OS X](tools/MacOSX/README.md), and [Windows](tools/WindowsREADME.md).


Planned features
----------------

### Features planned for 2.1 (ETA: End of May 2016)

- Integration of Blender's production proven point tracker in Natron to replace the existing TrackerPM node.

- Planar tracking of Rotoshapes

- Single Read/Write node instead of many Readers (ReadOIIO, ReadFFMPEG etc...)/Writers  (WriteOIIO, WriteFFMPEG...)

- Node-graph enhancement and optimization

- RotoPaint: add ability to use custom masks from inputs instead of a paint brush. Also improve the tree view with more per-shape attributes.

- Roto: add support for DopeSheet

### Features for 2.2 (ETA: End of july 2016)

- Optical Flow nodes: VectorGenerator, MotionBlur, RollingShutter, Retiming

- User manual and Reference guide


### Features planned for future versions

- 3D workspace: support for Cameras, 3D Cards, Camera mapping, 3D tracker

- GMIC http://gmic.eu integration as an OpenFX plug-in

- Natural matting: process of extracting a foreground without necessarily a green/blue-screen as background

- Deep data: Support for deep data (multiple samples per pixel)

Contributing
------------

We coordinate development through the [GitHub issue
tracker](https://github.com/NatronGitHub/Natron/issues).

The main development branch is called
["master"](https://github.com/NatronGitHub/Natron/tree/master).
The stable version is on branch RB-2.0.

Additionally each stable release supported has a branch on its own.
For example the stable release of the v1.0. and all its bug fixes should go into that
branch.
At some point,  version which are no longer supported will get removed from github's branches
and only a release tag will be available to get the source code at that point.

You can check out the easy tasks left to do [here](https://natron.fr/easy-task-list/).

Feel free to report bugs, discuss tasks, or pick up work there. If you want to make
changes, please fork, edit, and [send us a pull
request](https://github.com/NatronGitHub/Natron/pull/new/master),
preferably on the ["master"](https://github.com/NatronGitHub/Natron/tree/master)
branch.

There's a `.git-hooks` directory in the root. This contains a `pre-commit`
hook that verifies code styling before accepting changes. You can add this to
your local repository's `.git/hooks/` directory like:

    $ cd Natron
    $ mkdir .git/hooks
    $ ln -s ../../.git-hooks/pre-commit .git/hooks/pre-commit


Pull requests that don't match the project code style are still likely to be
accepted after manually formatting and amending your changeset. The formatting
tool (`astyle`) is completely automated; please try to use it.
