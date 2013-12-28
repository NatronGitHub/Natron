**UPDATED on 11/30/2013**

NATRON:

#OPEN-SOURCE,CROSSPLATFORM(OSX/LINUX/WINDOWS) , NODAL, COMPOSITING SOFTWARE UNDER MOZILLA PUBLIC LICENSE V2.0 (MPL)

Check out http://mrkepzie.github.io/Natron/ for a full presentation on what is Natron and the concepts.[This presentation is a bit outdated, a new one will be published soon (probably in november,before the 1st release)].

The development is going very well and most of the features are not pushed on the master branch yet.

Here's the state of the software as of today 11/30/2013:

- Support for dozens of OpenFX plugins: 

*https://github.com/devernay/openfx-misc
*https://sites.google.com/site/tuttleofx/
*http://www.thefoundry.co.uk/products/furnace/
*http://www.genarts.com/software/sapphire/overview
* and many more open-source openfx plugins !
- Lots of efforts are made to support a wide range of OpenFX plugins! 
Currently almost all features of OpenFX v1.3 are supported except OpenGL rendering enhancement, which shouldn't take long to implement;)
We don't come across many plugins not working on Natron now…;)
- Linux/OSX/Windows support.
- Platforms not supporting GLSL are also supported
- Fast Viewer interaction with no delay 
- Overlays interaction on the viewer
- Fast playback engine: possibility to run 32bit floating point 4K sequences at 60+ fps
- Reader node (file sequence reader) working natively for EXR's
- Writer node (rendering node) working natively for EXR's and faster than ever.
- Multi-rendering (simultaneously) is possible as well as  several viewers running playback simultaneously
- It is possible to separate on any number of screens the graphical user interface so that each viewer/graph editor belongs to one screen
- Several projects can be opened simultaneously in separate windows
- Auto-save support.
- Project format written in XML and easily editable by human.
- The project saves also the layout of the application.
- Command line tool for execution of project files. The command line version is executable from ssh on a computer without any display.
- Animation support and possibility to change it with a curve editor


##Features to come very soon## (most likely part of the V1):

- Real-time histograms,vectorscope,waveforms [All source code already exists,it's just a matter of hours to implement them]
- Graphical user interface colours customisation
- Proxy mode(i.e: downscaling of the input images to render faster)

##Features to come in V2##

- Dopesheet
- Progress report on the viewer (it is already implemented)
- Multi-view (http://imagine.enpc.fr/~moulonp/openMVG/) support.
- Meta-data support as well as per-plugin meta-data support by the node-graph
- Rotopainting node

##Features in long term##

- OpenGL rendering support to make processing nodes even faster
- 3D models viewer + renderer (using libQGLViewer)
- And many more features that are in the list but that I can't think off the top of my head!

We will first present a beta of Natron a work on a stable and mature version to produce the release of the version 1. Beta will open soon as well as a website for feedbacks, bug reporting etc…;)

