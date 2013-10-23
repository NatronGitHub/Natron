**UPDATED on 10/23/2013**

#OPEN-SOURCE,CROSSPLATFORM(OSX/LINUX/WINDOWS) , NODAL, COMPOSITING SOFTWARE UNDER
MOZILLA PUBLIC LICENSE V2.0 (MPL) #

Check out http://mrkepzie.github.io/Powiter/ for a full presentation on what is Powiter and the concepts.[This presentation is a bit outdated, a new one will be published soon (probably in november,before the 1st release)].

The development is going very well and most of the features are not pushed on the master branch yet.

Here's the state of the software as of today 10/23/2013:

- Support for dozens of OpenFX plugins: 

*https://sites.google.com/site/tuttleofx/
*http://www.thefoundry.co.uk/products/furnace/
*http://www.genarts.com/software/sapphire/overview
* and many more open-source openfx plugins !
- Lots of efforts are made to support a wide range of OpenFX plugins! (Temporal plugins are not yet supported but will soon be)
- Linux/OSX/Windows support.
- Platforms not supporting GLSL are also supported
- Fast Viewer interaction with no delay (like Nuke does) 
- Overlays interaction of the viewer
- Fast playback engine: possibility to run 32bit floating point 4K sequences at 60+ fps
- Reader node (file sequence reader) working natively for EXR's
- Writer node (rendering node) working natively for EXR's and faster than ever.
- Multi-rendering (simultaneously) is possible as well as  several viewers running playback simultaneously
- It is possible to separate on any number of screens the graphical user interface so that each viewer/graph editor belongs to one screen
- Several projects can be opened simultaneously in separate windows
- Auto-save support.
- Project format written in XML and easily editable by human.


##Features to come very soon## (most likely part of the V1):

- Command line tool for execution of project files [Shouldn't take a long time of dev since the software can currently operate without any GUI]
- Real-time (30fps+) histograms,vectorscope,waveforms [All source code already exists,it's just a matter of hours to implement them]
- Graphical user interface colours customisation
- Possibility to save the layout of the application in the project file
- Proxy mode(i.e: downscaling of the input images to render faster)
- KeyFrames support [It will maybe be part of V2]

##Features to come in V2##

- Progress report on the viewer (it is already implemented)
- TimeVarying OpenFX effects supports.
- Multi-view (http://imagine.enpc.fr/~moulonp/openMVG/) support.
- Meta-data support as well as per-plugin meta-data support by the node-graph
- Rotopainting node

##Features in long term##

- OpenGL rendering support to make processing nodes even faster
- Render farm
- And many more features that are in the list but that I can't think off the top of my head!


NB: The name of the project is still not decided and will soon change!
More publication will come when the name will be final.

The new website will come before the end of the year.

The software already supports a large part of OpenFX 1.2 features!  
The goal is to have a version of the software supporting all OpenFX 1.2 features for November 2013. Stay tuned for a lot of changes.
