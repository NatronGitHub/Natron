Natron <a id="top"></a>
=======

1. [Preamble](#Preamble)
    * Natron, What is this ?
    * Philosophy
    * For who?
    * Licensing
2. [Engine](#Engine)
    * Concepts
    	- Image processing
    	- Animation
    	- Rendering
    	- Rotoscoping/Rotopainting
    	- Interaction with other compositing softwares
3. [Graphical user interface](#GUI)
4. [Features in v1](#V1Features)
5. [Features in v2](#V2Features)
6. [Features in v3](#V3Features)

* * * * *

1. Preamble<a id="Preamble"></a>
-----------

####Natron,what is it? 
	
	Natron is an open-source, cross-platform (Windows,OSX,Linux), OpenFX 1.3 compatible
software designed for compositing and image processing.
Compositing is the process of combining visual elements from separate sources into single
images (wikipedia) .

####Philosophy<a id="Philosophy"></a>

We aim to provide a software on par with visual effects industry softwares performance
wise. We also want the end-user to be familiar with the Natron by dampening the learning
curve that exists when one first use a compositing software. This is done by using the
same looks and concepts as successful industries software. We clearly do not think Natron
is going to be a competitor to Nuke but we want to provide a software as close as the 
industry is used to use every day for making films. We design our software more as an
educational platform where junior artists/developers can get started using/developing
visual effects or image processing tools. Our goal is to extend the very closed
community of visual effects so new folks can come along and new movies can be released
with cheaper budgets!

That in mind, you may ask why not enhancing Blender's powerful compositing tool rather
than  creating a new software from scratch ?
Before creating Natron, we first took a glance at what open-source solutions are proposed
in the compositing field:
- Ramen is gone and we never had a chance to try it out ourselves
- Blender's compositing toolset
- TuttleOFX (a background OpenFX host and a suite of open-source plug-ins.)

Blender is probably the most well known open-source software in the 3D-modeling and 
animation field. This is not so true for its compositing module. I see 2 reasons for this:
- The graphical user interface is too complicated and is far from what industry softwares
interfaces actually look like (i.e The Foundry Nuke ,Adobe After Effects,eyeon Fusion,...)
- This open-source solution doesn't implement the standard open-source solution for making
visual effects plug-ins: OpenFX ! 
This is really game breaking, more and more professional studios are developing plug-ins
that are compatible on many compositing softwares. Today hundreds of industry-tested 
OpenFX plug-ins exist and Blender is turning its back to them. I don't see any
professional artist willing to give up the tools he's used to use every day and are
extremely performant to Blender-only tools. I think people don't want to be tied down
to 1 software anymore. With OpenFX the possibility to have a back-up software where you
can still get your work continued is a tremendous avantage.

####For who? 

The software is dedicated to everyone doing compositing, image processing ,
and that wants a competitive, robust, cutting edge toolset to use. First of all Natron is
dedicated towards educational purposes, that is: 
- junior artists
- junior developers in computer vision/computer graphics fields
- low budget films that couldn't access compositing before.

####Licensing 

Natron is under Mozilla Public License V2.

[Go back to the top of the page](#top)

2. Engine<a id="Engine"></a>
---------

###Concepts

Natron aims to to everything why a compositing software would be sought after:
- Image processing
- Animation
- Rendering
- Rotoscoping
- Interaction with others compositing software

####Image processing

Natron is a host for OpenFX image processing plug-ins. Plug-ins can be themselves chained
like a tree to produce a series of image transformations. This chain is composed of 3
important parts:
- Generating data
- Processing data
- Rendering data

The generation of data is done by reading plug-ins that read image or video file. Each
plug-in can be used to decode a particular file format (or even to use a specific library
like FFmpeg).

The processing part is the central part. It regroups all visual effects. The most basic
effect would be a blur, taking in input another effect and resulting in the image being
blurred.

Rendering can be done in 2 distinct ways:
1) By using an on-disk renderer plug-in. That is, a plugin that is dedicated to write a
specific tree of visual effects results on disk, in a particular file format. On-disk 
renderers are exactly the opposite of readers: They take in input an image and write the
image on disk.

2) By using Natron's built-in viewer which is able to monitor a compositing tree at any
step. It also provides the ability to monitor multiple steps at the same time to be able
to compare the image at different steps of the compositing tree. This viewer is not an 
OpenFX plug-in.

####Animation

Animation in a compositing software is quite simple. Taking a visual effects, we want it
to change over the time. For example we want to change the size of a blur across the time.
In practise the animation is represented like a curve representing for each time value 
(the X-axis) what value would our effect have (the Y-axis). Rest assured, the end user
doesn't have to provide her/himself a value for each individual time that exists, it 
would not be possible because the time is continuous! Rather the user provides values at
peaked times, known as keyframes, and the software interpolates the value beween those 
2 keyframes. 
Hence animation is not more than providing keyframes at each desired time value for 
a given parameter of a visual effect.


####Rendering

As said in the image processing part, rendering can be done in 2 ways:

1) on disk
2) by a viewer

Note that generally when talking about rendering with refer to 1), that is writing images
on disk.

In Natron the writers are made so they can be launched in another operating system's 
process. It prevents to crash any other instance of the application that would be 
rendering at the same time. 
Also the rendering can be launched as a background process (i.e: command-line) without
any graphical user interface. (It doesn't even require a X11 server on linux).
This allows the application to be launched by several computers at the same time (i.e: 
a render farm) with a simple shell script that would dispatch the total image sequences
among the available computers (e.g: PC 1 takes frames 0-99, PC 2 takes frames 100-199
etc...).

See the [command line manual](#cmdlineArgs) for more informations.

####Rotoscoping and rotopainting

Rotoscoping/painting is not yet implemented in Natron and will be for the version 2.


####Interaction with others compositing software

Natron implements OpenFX 1.3 standard. That is any compositing tree using only
OpenFX plug-ins should produce exactly the same results in Natron than in all other
OpenFX-compatible softwares (Assuming they implement all the features of the standard).

For now Natron has its own project file format. In the future we would like to be able
to have a simple script allowing to transfer a Natron's project to a Nuke's project.


[Go back to the top of the page](#top)

 3. Graphical user interface<a id="GUI"></a>
---------------------------

The graphical user interface of Natron is very similar to what The Foundry Nuke looks like.
We want to provide a software that is as close at what people are used to use in the
compositing field. For more insight on this topic please refer to the 
[philosophy](#Philosophy) of Natron.


[Go back to the top of the page](#top)


4. Features in V1<a id ="V1Features"></a>
-------------------

####OpenFX 1.3 support

####Animation support

####Background renderer

####Node-graph editor (to build compositing trees)

####Curve editor (for animation)

####Fast viewer to inspect any steps of the compositing tree

####Playback and disk cache for fast viewer results.

####Multi-view rendering support

####Human readable XML project file format.


5. Features in V2<a id ="V2Features"></a>
-------------------

####Real-time Histograms,Vectorscope,Waveforms

####Exportable project file's format to other compositing software's file format.

####Dopesheet

####Viewer progress report (maybe part of V1)

####Rotoscoping and Rotopainting support  (probably as  OpenFX plug-ins)
 
####Meta-datas support (probably as an OpenFX extension)


6- Features in V3<a id ="V3Features"></a>
------------------

####3D mode and multi-view support via libQGLViewer and
[OpenMVG](http://imagine.enpc.fr/~moulonp/openMVG/)

[Go back to the top of the page](#top)

