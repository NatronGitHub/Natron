developer installation for windows
==================================

This file is supposed to guide you step by step to have working (compiling) version of Powiter on windows. Right now, only a Visual studio 2010 solution file is proposed.

##Note

If you're not a Visual studio user, maybe in the future the QtCreator project file will get updated so it works on all platforms. For now you'd have to stick to Visual studio on windows, or make you're own project files.

If you're planning on writing a CMAKE based project file, and that you succeed in making it portable(windows/linux/osx), please send a mail at immarespond at gmail dot com.

##Install libraries

In order to have Powiter compiling, first you need to install the required libraries.

###Qt 5

You'll need to install Qt (5.0.2 as of writing) libraries from [Qt download](http://qt-project.org/downloads). Once downloaded, remember the path where you install it as we will need it later on.

###FFMPEG

You can retrieve pre-built binaries from (ffmpeg zeranoe build)[http://ffmpeg.zeranoe.com/builds/] . Select the developer files for the version of your system (32/64bits).


###OpenEXR 2.0.0

This one is not easy. You have two solutions, either you can get the binaries I compiled for windows on dropbox, or you can download it from :
(OpenEXR on github)[https://github.com/openexr/openexr] and compile it yourself. Warning : the .sln file provided to compile openexr does not include all the source files (especially the recently added deep compositing source files).

Here's the dropbox link for 32bit binaries :(Dropbox link)[https://www.dropbox.com/sh/k3t7ycow3a45e54/P3pg62FCBr]

(64bit) will come soon.

###freetype2


Compile it by yourself from (Freetype download)[http://download.savannah.gnu.org/releases/freetype/] (make sure to download freetype 2.x). It should be pretty much straight forward. Powiter does a static link with freetype, make sure to build it as a static library.

###ftgl

Download it and compile it yourself from (FTGL download)[http://sourceforge.net/projects/ftgl/]. It should be pretty much straight forward. Powiter does a static link with freetype, make sure to build it as a static library.


###boost

Boost headers are required, but not the libraries. You can download boost from 
(boost download)[http://www.boost.org/users/download/]

We're done here for libraries.

##Setting the environment variables

We'll need to set a few windows environment variables so that the solution file recognises the path to the libraries. 

Here they are:

BOOST_INCLUDES
GLEW_INCLUDES
GLEW_LIB
GLEW_BIN
FFMPEG_BIN
FFMPEG_INCLUDES
FFMPEG_LIB
FREETYPE2_INCLUDES
FREETYPE_INCLUDES
FREETYPE2_LIB
FTGL_INCLUDES
FTGL_LIB
OPENEXR_BIN
OPENEXR_INCLUDES
OPENEXR_LIB
QT_LIB
QT_INCLUDES
QT_MOC_PATH

You'll need to change the path of these variable accordingly to the locations of the includes/libraries on your computer. 

A few notes:

*QT_MOC_PATH is the path to the moc tool so the script can find it when compiling

*All XXX_INCLUDES variables reference the place where the include files can be found for the library XXX.

*All XXX_LIB variables reference where the .lib files can be found for the library XXX.

*All XXX_BIN variables reference where the .dll files can be found for the library XXX.

That should be all, Powiter should now compile and run.

##Adding QT classes using signals/slots

You'll have to define the Q_OBJECT macro in the declaration of the class so the moc tool will actually "moc" it.
In order to make it work with visual studio you'll have to do these step for the header file :

- right click on the header file in the tree => Properties.
- Select "All configurations" for the combobox 'configuration'.
- Click the custom build tool tab in the left panel.
- add this line to the command line :
	$(QT_MOC_PATH)\moc.exe  %(FullPath) -f -o C:\%(Directory)..\ProjectFiles\GeneratedFiles\%(Filename)_moc.cpp
- add whatever you want to the description
- add this line to the outputs:
	C:\%(Directory)..\ProjectFiles\GeneratedFiles\%(Filename)_moc.cpp
- and finally add this line to the additional dependencies:
	$(QT_MOC_PATH)\moc.exe;$(InputPath)


