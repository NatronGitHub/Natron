developer installation for windows
==================================

Required: Install git, best is the github app.

##Install libraries

In order to have Natron compiling, first you need to install the required libraries.
An alternative is to download the pre-compiled binaries provided by MrKepzie from
[Dropbox](https://www.dropbox.com/s/s5yuh9k3kum99jp/3rdparty_windows_32_and_64bits_msvc2010.zip)
They contain:
* Qt 4.8.5 compiled for 64 bits as dlls. 32 bits version can be downloaded from the Qt website (see below)
* lib jpeg 8d/9a static MT. You should use the 9a version for OpenImageIO
* libpng 1.2.51/ 1.6.9 static MT, you should use version 1.6.9 for OpenImageIO and cairo
* OpenEXR 2.1 static MT
* OpenImageIO 1.4.2 static MT release
* libtiff 4.0.3 static MT release
* zlib 1.2.8 static MT release
* OpenColorIO 1.0.9 static MT release
* cairo 1.12.4 static MT release
* pixman 0.32.4 static MT release (64 bits only, it's easy to compile the 32 bits version yourself).

###*Qt 4.8*

You'll need to install Qt libraries from [Qt download](http://qt-project.org/downloads). 

###*boost*

You can download boost from 
(boost download)[http://www.boost.org/users/download/]
For now only boost serialisation is required. Follow the build instructions to compile
boost. You'll need to build a shared | multi-threaded version of the librairies.
Pre-compiled binaries for boost are available here:
http://boost.teeks99.com/

###*glew*
On windows,OpenGL API is not up to date to the features we need.
You'll need to download glew, that provides a recent version of the OpenGL API:

(Glew download)[http://glew.sourceforge.net/]


###*OpenFX*

At the source tree's root, type the following command in the command prompt:

	git submodule update -i --recursive
	
(If you cloned the repository with the github app you probably don't need this command.)

###*Expat*

You need to build expat in order to make OpenFX work.
The expat sources are located under the HostSupport folder within OpenFX.
You should find a .vcxproj file under
\libs\OpenFX\HostSupport\expat-2.0.1\lib
Open it with visual studio and build a static MT release version of expat.

We're done here for libraries.

###*Cairo 1.12*

You can get the release from http://www.cairographics.org/releases/
Then this page has a complete answer on how to compile the dependencies:
http://stackoverflow.com/questions/85622/how-to-compile-cairo-for-visual-c-2008-express-edition
This will work successfully.

###Add the config.pri file

You have to define the locations of the required libraries.
This is done by creating a .pri file that will tell the .pro where to find those libraries.

- create the config.pri file next to the Project.pro file.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

*INCLUDEPATH* is the path to the include files
*LIBS* is the path to the libs

Here's an example of a config.pri file:
---------------------------------------

	boost {

    	INCLUDEPATH +=  $$quote(C:\\local\\boost_1_55_0_vs2010_x86)
    
	    LIBS += $$quote(C:\\local\\boost_1_55_0_vs2010_x86\\lib\\boost_serialization-vc100-mt-1_55.lib)
	    
	}


	glew {
	
    	INCLUDEPATH +=  $$quote(C:\\local\\glew\\include)
    	
	    LIBS += -L$$quote(C:\\local\\glew\\lib\\Release\\Win32\\glew32.lib)
	    
	}

	expat
	{
	
		INCLUDEPATH += $$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron\\libs\\OpenFX\\HostSupport\\expat-2.0.1\\lib)
		
		LIBS += -L$$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron\\libs\\OpenFX\\HostSupport\\expat-2.0.1\\win64\\bin\\Release) -llibexpatMT
		
		LIBS += shell32.lib	
	}

	
	cairo {
	
		INCLUDEPATH +=  $$quote(path\\to\\cairo\\includes)
    
	    LIBS += -L$$quote(path\\to\\cairo\\lib) -lcairo
	}

---------------------------------------------

###*Copy the dll's over*

Copy all the required dll's to the executable directory. (Next to Natron.exe)
If Natron launches that means you got it all right;) Otherwise windows will prompt you
for the missing dll's when launching Natron.

###*Build*


	C:\Qt\4.8.5\bin\qmake.exe -r -tp vc -spec win32-msvc2010 Project.pro

(adjust the qmake executable path to your system).

The vcproj "Natron" might complain of missing includes or linkage errors, if so adjust
the settings in the Additional include directories and Additional dependencies tab of
the property page of the project.

If you get the following linker error:
error LNK2019: unresolved external symbol WinMain referenced in function __tmainCRTStartup
Open the Natron project property pages. Go to Configuration Properties --> Linker --> Command Line.
In the Additional Options, add the following: 
     /ENTRY:"mainCRTStartup" 
(add a white space after the previous command)


