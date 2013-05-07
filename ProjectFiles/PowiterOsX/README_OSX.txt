Developer installation on mac osx
=================================

This file is supposed to guide you step by step to have working (compiling) version of Powiter on mac osx ( >= 10.6 ). This is done using Xcode. You should have an up-to-date version of Xcode (as of writing 4.6.2) with all the mac osx core library downloaded.
You'll also need an up to date macports version. Just download it and install it from : 
(Macports website)[http://www.macports.org]

##Note

If you're not a Xcode user, maybe in the future the QtCreator project file will get updated so it works on all platforms. For now you'd have to stick to Xcode on mac, or make you're own project files.

If you're planning on writing a CMAKE based project file, and that you succeed in making it portable(windows/linux/osx), please send a mail at immarespond at gmail dot com.

##Install libraries

In order to have Powiter compiling, first you need to install the required libraries.

###Qt 4.8

You'll need to install Qt 4.8 libraries from [Qt download](http://qt-project.org/downloads). Select the "Qt libraries 4.8.4 for Mac (185 MB)"
Powiter will most likely upgrade to Qt5 soon, but for now it's not compatible with Qt5 fully. Once downloaded, remember the path where you install it as we will need it into Xcode.

###FFMPEG

Open a terminal and type:
	sudo port install ffmpeg-devel
It should install it to your macports download tree. By default it should be something along the lines of /opt/local 

###OpenEXR 2.0.0

This one is not on macports yet. You have two solutions, either you can get the binaries I compiled for osx 64bit on dropbox, or you can download it from :
(OpenEXR on github)[https://github.com/openexr/openexr] and compile it yourself.

Here's the dropbox link for 64bit binaries :(Dropbox link)[https://www.dropbox.com/sh/ru7cjns0fjvn7aa/U8iRn3xQEn]

###freetype2

You can either use macports:
	sudo port install freetype
or compile it by yourself from (Freetype download)[http://download.savannah.gnu.org/releases/freetype/] (make sure to download freetype 2.x)

###ftgl

Again with macports:
	sudo port install ftgl
Or download it and compile it yourself from (FTGL download)[http://sourceforge.net/projects/ftgl/]

###boost

Boost headers are required, but not the libraries. You can download boost from 
(boost download)[http://www.boost.org/users/download/]
or through macports, but it will also install the boost libraries:
	sudo port install boost

We're done here for libraries.

##Configure the Xcode project

Now that you have all the libs, you need to go open the Xcode project under Powiter_Core/ProjectFiles/PowiterOsX/
Open the Xcode preferences menu (cmd,) and go to Locations. Under the tab Source Trees
you'll see all environment variables that Powiter uses to build.
If they do not appear, here are the variables:

BOOST_INCLUDES
FFMPEG_INCLUDES
FFMPEG_LIB
FREETYPE2_INCLUDES
FREETYPE_INCLUDES
FTGL_INCLUDES
FTGL_LIB
OPENEXR_INCLUDES
OPENEXR_LIB
QT_LIB
QT_INCLUDES
QT_MOC_PATH

You'll need to change the path of these variable accordingly to the locations of the includes/libraries on your computer. 



A few notes:

*QT_MOC_PATH is the path to the moc tool so the script can find it when compiling

*All XXX_INCLUDES variables reference the place where the include files can be found for the library XXX.

*All XXX_LIB variables reference where the .dylib or .a files can be found for the library XXX.

##Configure project schemes

Still in Xcode, in the Product menu, go into the Scheme sub-menu, click Edit scheme. . .(cmd <). In the left menu and select "Run Powiter". 
In the info tab , change the executable to Powiter. Then go to the option tab and select "Working directory". You need to put for working directory the base of Powiter_Core: e.g
 
/Users/blabla/Dev/Powiter_Core


That should be all, you can select the target "Powiter" and compile. 



##Adding QT classes using signals/slots

You'll have to define the Q_OBJECT macro in the declaration of the class so the moc tool will actually "moc" it.
In order to make it work with XCode  you'll have to add your header to the compiled sources of the target PowiterOsX ( in the tab Build Phases ).



