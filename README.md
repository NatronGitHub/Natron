Natron [![Build Status](https://api.travis-ci.org/MrKepzie/Natron.png?branch=workshop)](https://travis-ci.org/MrKepzie/Natron)  [![Coverage Status](https://coveralls.io/repos/MrKepzie/Natron/badge.png?branch=workshop)](https://coveralls.io/r/MrKepzie/Natron?branch=workshop) [![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/MrKepzie/Natron/trend.png)](https://bitdeli.com/free "Bitdeli Badge")
======


Natron is an Open-Source (MPLv2 license) video compositing software, similar in functionality to Adobe After Effects or Nuke by The Foundry.

It is portable and cross-platform (Linux, OS X, Microsoft Windows).

The project home page is http://natron.inria.fr

The project source code repository is https://github.com/MrKepzie/Natron

Features
--------

- Support for many free and open-source OpenFX plugins: 
 * [TuttleOFX](https://sites.google.com/site/tuttleofx/)
 * [OpenFX-IO](https://github.com/MrKepzie/openfx-io) to read anything else
   than standard 8-bits images
 * [OpenFX-Misc](https://github.com/devernay/openfx-misc)
 * [OpenFX-Yadif deinterlacer](https://github.com/devernay/openfx-yadif)
 * [OpenFX-Vegas SDK samples](https://github.com/devernay/openfx-vegas)
 * [OpenFX samples](https://github.com/devernay/openfx) (in the Support and Examples directories)

- Support for commercial OpenFX plugins:
 * [Furnace by The Foundry](http://www.thefoundry.co.uk/products/furnace/)
 * [KeyLight by The Foundry](http://www.thefoundry.co.uk/products/plugins/keylight/)
 * [GenArts Sapphire](http://www.genarts.com/software/sapphire/overview)
 * [Other GenArts products](http://www.genarts.com/software/other-vfx-products)
 * And probably many more. Please tell us if you successfully tested other commercial plugins.

- OpenFX: Currently almost all features of OpenFX v1.3 are supported
  (see Documentation/ofxActionsSupported.rtf and
  Documentation/ofxPropSupported.rtf in the source distribution)
  
- Linear 32-bits floating point workflow: all images are internally stored with linear colors
and with floating point values for best results.
  
- A minimum of OpenGL 1.5 is required, hence most graphics cards should be supported, 
platforms not supporting GLSL are also supported
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
- OpenColorIO support via the plug-ins located in  [OpenFX-IO](https://github.com/MrKepzie/openfx-io)
- OpenImageIO support via the plug-ins located in  [OpenFX-IO](https://github.com/MrKepzie/openfx-io)

Requirements
------------

A machine running one of the supported operating systems (Linux, OS X,
Microsoft Windows), and a 32-bits x86 or 64-bits x86-64 processor.

If your OpenGL version is not supported or does not implement the
required extensions, you will get an error when launching Natron for
the first time.

The system must support one of these OpenGL configurations:
- OpenGL 2.0
- OpenGL 1.5 with the extensions `GL_ARB_texture_non_power_of_two`
  `GL_ARB_shader_objects` `GL_ARB_vertex_buffer_object`
  `GL_ARB_pixel_buffer_object`


Planned features
----------------

### Features planned for next major version

- Dopesheet
- Rotopainting/scoping nodes
- Python scripting

### Features planned for future versions

- Multi-view (http://imagine.enpc.fr/~moulonp/openMVG/) support.
- Meta-data support as well as per-plugin meta-data support by the node-graph
- Graphical user interface colours customisation
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

Additionally each stable release supported has a branch on its own.
For example the stable release of the v1.0. and all its bug fixes should go into that 
branch.
At some point,  version which are no longer supported will get removed from github's branches
and only a release tag will be available to get the source code at that point.

You can check out the easy tasks left to do [here](https://natron.inria.fr/easy-task-list/).

Feel free to report bugs, discuss tasks, or pick up work there. If you want to make
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
