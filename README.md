Natron [![Build Status](https://api.travis-ci.org/MrKepzie/Natron.png?branch=workshop)](https://travis-ci.org/MrKepzie/Natron)  [![Coverage Status](https://coveralls.io/repos/MrKepzie/Natron/badge.png?branch=workshop)](https://coveralls.io/r/MrKepzie/Natron?branch=workshop) [![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/MrKepzie/Natron/trend.png)](https://bitdeli.com/free "Bitdeli Badge")
======


Natron is an Open-Source (MPLv2 license) video compositing software, similar in functionality to Adobe After Effects or Nuke by The Foundry.

It is portable and cross-platform (Linux, OS X, Microsoft Windows).

The project home page is http://natron.inria.fr

The project source code repository is https://github.com/MrKepzie/Natron

Features
--------

- 32 bits floating point linear colour processing pipeline.
- Colorspace management handled by the famous open-source OpenColorIO library.
- Dozens of file formats supported: EXR, DPX,TIFF, JPG, PNG…thanks to OpenImageIO.
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

- Intuitive user interface: Natron aims not to break habits by providing an intuitive and familiar user
interface.  It is possible to separate on any number of screens the graphical user interface.

- Performances:  Never wait for anything to be rendered, in Natron anything you do produces
real-time feedback thanks to its optimised multi-threaded rendering pipeline and its support for proxy rendering (i.e:
the render pipeline can be computed at lower res to speed-up rendering).

- Multi-task: Natron can render multiple graphs at the same time, it can also be used
as a background process in command-line mode without any display support (e.g: for render farm purpose).

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
with a very accurate and intuitive curve editor.

- Command line tool for execution of project files. The command line version is executable
 from ssh on a computer without any display. Hence it is possible to use a render farm
  to render Natron's projects. 
  
- Multi-view workflow: Natron saves time by keeping all the views in the same stream. You can separate
the views at any time with the SplitViews node. Note that currently Natron does not allow to split the 
nodes settings for each view, this will be implemented in the future.

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

- Mask edition via a rotoscoping node: We're half way through the development of this feature.
- Chroma keyer: This feature is implemented and usable.

### Features planned for future versions

- Node-graph enhancements: "global view" + magnetic grid + pre-comps

- Python scripting: Natron will be entirely scriptable, to operate with the node graph faster and
also to allow Natron to be used as a command-line tool only.

- Dope sheet: Well this is time we implement this, it can be very tedious to organise image sequences 
without this very useful tool.

- Presets: As the scripting will be implemented, it will be easy for us to add Node presets that you can
share with others.

- Templates: Template nodes are an aggregation of several nodes put together as a graph that act
like a simple node. Share your templates and save time re-using them as part of other graphs.

- Deep data: Support for deep data (multiple samples per pixel)

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
