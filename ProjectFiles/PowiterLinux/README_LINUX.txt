developer installation for linux
==================================

This file is supposed to guide you step by step to have working (compiling) version of Powiter on linux. Right now, only a QtCreator project file is proposed.

##Note

If you're not a QtCreator user, maybe in the future a CMAKE based project version will be available.
If you're planning on writing a CMAKE based project file, and that you succeed in making it portable(windows/linux/osx), please send a mail at immarespond at gmail dot com.

##Install libraries

In order to have Powiter compiling, first you need to install the required libraries.

###Qt 4.8

You'll need to install Qt 4.8 libraries from [Qt download](http://qt-project.org/downloads). Select the "Qt libraries 4.8.4 for Linux/X11 (225 MB)  "
Powiter will most likely upgrade to Qt5 soon, but for now it's not compatible with Qt5 fully. Once downloaded, you'll have to compile it. Another option is to install it with your packet manager: e.g:
	sudo apt-get install qt4

###FFMPEG

You'll need to get ffmpeg-devel from the packet manager : 
	sudo apt-get install ffmpeg-devel

###OpenEXR 2.0.0

You have two solutions, either you can get the binaries I compiled for linux on dropbox, or you can download it from :
(OpenEXR on github)[https://github.com/openexr/openexr] and compile it yourself. 

Here's the dropbox link for 32bit binaries :(Dropbox link)[https://www.dropbox.com/sh/a42hus72e39wf3g/WkRPPcR1TS]

(64bit) will come soon.

###freetype2

You can get it from the packet manager:
	sudo apt-get install freetype2


###ftgl

Again, you can get it from apt-get:
	sudo apt-get install ftgl


###boost

Boost headers are required, but not the libraries. You can download boost from 
(boost download)[http://www.boost.org/users/download/]
Alternatively you can install boost from apt-get:
	sudo apt-get install libboost1.50-all-dev

We're done here for libraries.

##Setting the environment variables

Open the PowiterWrapper.pro file under Powiter_Core/ProjectFiles/PowiterLinux/PowiterWrapper
We'll need to set a few qtcreator environment variables so that the solution file recognises the path to the libraries. Go into the "projects tab", and click the "PowiterWrapper" tab. In the build tab, go into the build environment section. Here just change the value of the following variables:

BOOST_INCLUDES
FFMPEG_INCLUDES
FFMPEG_LIB
FREETYPE2_INCLUDES
FREETYPE_INCLUDES
FREETYPE2_LIB
FTGL_INCLUDES
FTGL_LIB
OPENEXR_INCLUDES
OPENEXR_LIB
QT_INCLUDES


You'll need to change the path of these variable accordingly to the locations of the includes/libraries on your computer. 

A few notes:


*All XXX_INCLUDES variables reference the place where the include files can be found for the library XXX.
A few exceptions : OPENEXR_INCLUDES must reference the OpenExr folder and not the one containing that folder.

*All XXX_LIB variables reference where the .a,.so files can be found for the library XXX.

Still in the PowiterWrapper tab in the "projects" section, change the build directory to : 
YOUR_PATH_TO_POWITER_CORE/Powiter_Core/ProjectFiles/PowiterLinux/build

Select the "run" tab, change the working directory to :
YOUR_PATH_TO_POWITER_CORE/Powiter_Core/

Now go back to the "Edit" section. Right click on PowiterWrapper and build. It should build, but we cannot run it. Somehow the working directory is not taken into account when running. You'll have to open the "Powiter.pro" project separatly to run it:
File->open file or project : select Powiter.pro under Powiter_Core/ProjectFiles/PowiterLinux/PowiterWrapper/Powiter
Go into the projects section. In the run tab, change the working directory to :
YOUR_PATH_TO_POWITER_CORE/Powiter_Core/

You have to add all the ***_INCLUDES environment variables above again in the Build environment section of the Powiter project.

Now you can right click on the Powiter project (not the one in the PowiterWrapper, but the one you just opened) and select run.

If you want to build, do so with PowiterWrapper. If you want to run, do so with the Powiter project.

That should be all.

