Natron
======

Natron is an Open-Source (MPLv2 license) video compositing software, similar in functionality to Adobe After Effects or Nuke by The Foundry.

It is portable and cross-platform (Linux, OS X, Microsoft Windows).

The project home page is http://natron.inria.fr

Features
--------

- Support for many open-source and commercial OpenFX plugins: 

 * https://github.com/devernay/openfx-misc
 * https://sites.google.com/site/tuttleofx/
 * http://www.thefoundry.co.uk/products/furnace/
 * http://www.genarts.com/software/sapphire/overview
 * and many more...

- OpenFX: Currently almost all features of OpenFX v1.3 are supported
- Linux/OSX/Windows support.
- Platforms not supporting GLSL are also supported
- Fast Viewer interaction with no delay 
- Overlays interaction on the viewer
- Fast playback engine: possibility to run 32bit floating point 4K sequences at 60+ fps
- Multi-rendering (simultaneously) is possible as well as  several viewers running playback simultaneously
- It is possible to separate on any number of screens the graphical user interface so that each viewer/graph editor belongs to one screen
- Several projects can be opened simultaneously in separate windows
- Auto-save support.
- Project format written in XML and easily editable by human.
- The project saves also the layout of the application.
- Command line tool for execution of project files. The command line version is executable from ssh on a computer without any display.
- Animation support and possibility to change it with a curve editor

Planned features
----------------

### High priority

- Real-time histograms,vectorscope,waveforms [All source code already exists,it's just a matter of hours to implement them]
- Graphical user interface colours customisation
- Proxy mode(i.e: downscaling of the input images to render faster)

### Features planned for next major version

- Dopesheet
- Progress report on the viewer (it is already implemented)
- Multi-view (http://imagine.enpc.fr/~moulonp/openMVG/) support.
- Meta-data support as well as per-plugin meta-data support by the node-graph
- Rotopainting node

### Features planned for future versions

- OpenGL rendering support to make processing nodes even faster
- 3D models viewer + renderer (using libQGLViewer)
- And many more features that are in the list but that I can't think off the top of my head!

Contributing
------------

We coordinate development through the [GitHub issue
tracker](https://github.com/MrKepzie/Natron/issues).

The main development branch is called
["workshop"](https://github.com/MrKepzie/Natron/tree/workshop).
The master branch contains the last known stable version.

Feel free to
report bugs, discuss tasks, or pick up work there. If you want to make
changes, please fork, edit, and [send us a pull
request](https://github.com/MrKepzie/Natron/pull/new/workshop),
preferably on the ["workshop"](https://github.com/MrKepzie/Natron/tree/workshop)
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
