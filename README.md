# Natron

[![GPL2 License](http://img.shields.io/:license-gpl2-blue.svg?)](https://github.com/NatronGitHub/Natron/blob/master/LICENSE.txt) [![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v1.4%20adopted-ff69b4.svg)](CODE_OF_CONDUCT.md) [![Build Status](https://api.travis-ci.org/NatronGitHub/Natron.svg?branch=RB-2.4)](https://travis-ci.org/NatronGitHub/Natron) [![Coverage Status](https://coveralls.io/repos/NatronGitHub/Natron/badge.svg?branch=master)](https://coveralls.io/r/NatronGitHub/Natron?branch=master) [![Documentation Status](https://readthedocs.org/projects/natron/badge/?version=rb-2.4)](http://natron.readthedocs.io/en/rb-2.4/) [![Packaging status](https://repology.org/badge/tiny-repos/natron.svg)](https://repology.org/project/natron/badges) [![BountySource Status](https://api.bountysource.com/badge/team?team_id=271309)](https://www.bountysource.com/teams/natrongithub/issues?utm_source=Mozilla&utm_medium=shield&utm_campaign=bounties_received) [![OpenHub](https://www.openhub.net/p/natron/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/Natron)

---

Natron is a free, open-source (GPLv2 license) video compositor, similar in functionality to Adobe After Effects, Foundry's Nuke, or Blackmagic Fusion. It is portable and cross-platform (GNU/Linux, macOS, and Microsoft Windows).

- Website: https://natrongithub.github.io
- Source code: https://github.com/NatronGitHub/Natron
- Forum: https://discuss.pixls.us/c/software/natron
- Discord: https://discord.gg/cpMj5p3Fv5
- User documentation: https://natron.readthedocs.io/

## Help wanted

Natron is looking for developers and maintainers! You can help develop and maintain Natron if you have the following skills:

- [Git](https://en.wikipedia.org/wiki/Git) and [GitHub](https://en.wikipedia.org/wiki/GitHub)
- [C++](https://en.wikipedia.org/wiki/C%2B%2B) (Natron source is still C++98, but switching to [C++11](https://en.wikipedia.org/wiki/C%2B%2B11) or [C++14](https://en.wikipedia.org/wiki/C%2B%2B11) should be straightforward if needed)
- [Design patterns](https://en.wikipedia.org/wiki/Software_design_pattern)
- [Qt](https://www.qt.io/) (Natron builds with Qt4 or Qt5, but does not yet support Qt6)
- Basic knowledge of [OpenGL](https://en.wikipedia.org/wiki/OpenGL)
- Basic knowledge of [Python](<https://en.wikipedia.org/wiki/Python_(programming_language)>)

For more information, see the "Contributing" section below.

If you are willing to help, please contact the development team on the [pixls.us Natron forum](https://discuss.pixls.us/c/software/natron).

## Features

- 32-bit floating-point linear color processing pipeline.
- Color management handled by [OpenColorIO](https://opencolorio.org/).
- Dozens of video and image formats supported such as: H264, DNxHR, EXR, DPX, TIFF, JPG, PNG through [OpenImageIO](https://github.com/OpenImageIO/oiio) and [FFmpeg](https://ffmpeg.org/).
- Support for many free, open-source, and commercial OpenFX plugins—currently almost all features of OpenFX v1.4 are supported. Those marked with (+) are included in the binary releases.
  - [OpenFX-IO](https://github.com/NatronGitHub/openfx-io) (+)
  - [OpenFX-Misc](https://github.com/NatronGitHub/openfx-misc) (+)
  - [OpenFX-G'MIC](https://github.com/NatronGitHub/openfx-gmic) (+)
  - [OpenFX-Arena](https://github.com/NatronGitHub/openfx-arena) (+)
  - [All OFX products from RevisionFX](http://www.revisionfx.com)
  - [Boris FX](https://borisfx.com/) OpenFX plugins, including Sapphire
  - [Furnace by The Foundry](http://www.thefoundry.co.uk/products/furnace/)
  - ...And many more! Please tell us if you successfully tested other commercial plugins.
- Intuitive user interface: Natron aims not to break habits by providing an intuitive and familiar user interface. It is possible to customize and separate the graphical user interface on any number of screens. You can re-use your layouts and share your layout files (.nl).
- Performance: In Natron, anything you do produces real-time feedback in the viewer thanks to the optimized multi-threaded rendering pipeline and support for proxy rendering (computing at a lower resolution to speed up rendering).
- Multi-task: Natron can render multiple graphs at the same time. It can also be used as a background process in headless mode.
- Recover easily from bugs: Natron's auto-save system detects inactivity and saves your work for yourself. Natron is also able to render frames in a separate process, meaning that any crash in the main application would not crash the ongoing render (and the other way around).
- Project files saved in XML and easily editable by humans.
- Fast & interactive viewer - Smooth & accurate zooming/panning even for very large image sizes (tested on 27k x 30k images).
- Real-time playback: Natron offers real-time playback with excellent performance thanks to its RAM/Disk cache. Once a frame is rendered it can be reproduced instantly afterward, even for large image sizes.
- Animate your visual effects: Natron offers a simple and efficient way to deal with keyframes with a very accurate and intuitive Curve Editor as well as a Dope Sheet to quickly edit your motion graphics.
- Command-line rendering: Natron is capable of running without a GUI for batch rendering with scripts or on a render farm.
- Rotoscoping, rotopainting, and tracking support
- Multi-view workflow: Natron saves time by keeping all the views in the same stream. You can separate the views at any time with the OneView node.
- Python scripting integration:

  - Parameters expressions
  - User-defined parameters
  - Nodes groups as Python scripts
  - Script editor to control the application overall
  - User-defined python callbacks to respond to particular checkpoints of the internals of the software (change of a parameter, before rendering a frame, etc…)
  - Integration of Pyside to the GUI so that the interface is extensible with new menus and windows

- Multi-channel compositing: Natron can manipulate multi-layered EXR files thanks to OpenImageIO. Users can choose to work with any layer or channel on any node, new custom layers can also be created.

## Requirements

A machine running one of the supported operating systems (GNU/Linux, macOS, Microsoft Windows), and a 32-bits x86 or 64-bits x86-64 processor.

An OpenGL 2.0 compatible graphics card is needed to run Natron (2.1+) with hardware-accelerated rendering. Other graphics cards work with software-only rendering (see below).

The following graphics cards are supported for hardware-accelerated rendering:

- Intel GMA 3150 (Linux-only)
- Intel GMA X3xxx (Linux-only)
- Intel GMA X4xxx (Windows 7 & Linux)
- Intel HD (Ironlake) (Windows 7 & Linux)
- Intel HD 2000/3000 (Sandy Bridge) (Windows 7/Linux/Mac)
- Intel HD 4000 and greater (All platforms)
- Nvidia GeForce 6 series and greater
- Nvidia Quadro FX and greater
- Nvidia Quadro NVS 285 and greater
- ATI/AMD Radeon R300 and greater
- ATI/AMD FireGL T2-64 and greater (FirePro)

On Windows and Linux you can enable software rendering. On Linux, enable the environment variable LIBGL_ALWAYS_SOFTWARE=1 before running Natron. On Windows, enable the legacy hardware package in the installer.

## Installing

### Binary distribution

Standalone binary distributions of Natron are available for GNU/Linux, Windows, and macOS on [GitHub](https://github.com/NatronGitHub/Natron/releases), or from [the Natron web site](https://natrongithub.github.io/#download). These distributions contain Natron and four included sets of OpenFX plugins:

- [openfx-io](https://github.com/NatronGitHub/openfx-io/)
- [openfx-misc](https://github.com/NatronGitHub/openfx-misc)
- [openfx-arena](https://github.com/NatronGitHub/openfx-arena)
- [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic)

Alternatively, on Linux systems you can install Natron through flatpak: ``` flatpak install fr.natron.Natron ```

For each architecture / operating system, you can either download a stable release, a release candidate (if available), or one of the latest snapshots. Note that snapshots contain the latest features and bug fixes, but may be unstable.

### Building and installing from source

There are instructions for building Natron and the basic plugins from source is this directory on various architectures / operating systems:

- [GNU/Linux](INSTALL_LINUX.md)
- [macOS](INSTALL_MACOS.md)
- [FreeBSD](INSTALL_FREEBSD.md)
- [Windows](INSTALL_WINDOWS.md)

This documentation may be slightly outdated, so do not hesitate to submit updated build instructions, especially for the various GNU/Linux distributions.

### Automatic build scripts & other development tools

These can be found in [tools/README.md](tools/README.MD)

These scripts run on virtual machines running a specific operating system, setting these up is more complicated than the basic build process linked above.

## Contributing

### Low hanging fruits

You should start contributing to the Natron project by first picking an easy task, and then gradually taking more difficult tasks. Here are a few sample tasks, by order of difficulty (from 0 to 10):

- 2: Pyplugs, Shadertoy scripts (there are still developers for these, see https://github.com/NatronGitHub/natron-plugins )
- 4: Write an OpenFX plugin, starting from an example in [openfx-misc](https://github.com/NatronGitHub/openfx-misc) or from the [official OpenFX](https://github.com/NatronGitHub/openfx) examples, for example try to make an OpenFX plugin from a widely-used PyPlug. There are a few OFX plugin developers in the community.
- 5: Build Natron locally (on any system)
- 7: Compile a redistributable Natron binary (Linux is easier since we build and ship most dependencies using the build scripts)
- 9: Fix a simple Natron bug
- 10: Add new functionality to Natron (see issues)

### Logistics

We coordinate development through the [GitHub issue tracker](https://github.com/NatronGitHub/Natron/issues).

The main development branch is called ["master"](https://github.com/NatronGitHub/Natron/tree/master). The stable version is on branch RB-2.4.

Additionally, each stable release supported has a branch on its own. For example, the stable release of the v1.0. and all its bug fixes should go into that branch. At some point, a version that is no longer supported will get removed from GitHub's branches and only a release tag will be available to get the source code at that point.

Feel free to report bugs, discuss tasks, or pick up work there. If you want to make changes, please fork, edit, and [send us a pull request](https://github.com/NatronGitHub/Natron/pull/new/RB-2.4), preferably on the ["RB-2.4"](https://github.com/NatronGitHub/Natron/tree/RB-2.4) branch.

There's a `.git-hooks` directory in the root. This contains a `pre-commit` hook that verifies code styling before accepting changes. You can add this to your local repository's `.git/hooks/` directory by doing the following:

```shell
cd Natron
mkdir .git/hooks
ln -s ../../.git-hooks/pre-commit .git/hooks/pre-commit
```

Pull requests that don't match the project code style are still likely to be accepted after manually formatting and amending your changeset. The formatting tool (`astyle`) is completely automated; please try to use it.
