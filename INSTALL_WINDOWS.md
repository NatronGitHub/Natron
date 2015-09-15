developer installation for windows
==================================

Required: Install git, best is the github app.

##Install libraries

In order to have Natron compiling, first you need to install the required libraries.
An alternative is to download the pre-compiled binaries provided by MrKepzie from
https://sourceforge.net/projects/natron/files/Natron_Windows_3rdParty.zip/download
They contain:
* Qt 4.8.5 compiled for 64 bits as dlls. 32 bits version can be downloaded from the Qt website (see below)
* Qt 5.3.0  compiled for 64 bits as dlls. 32 bits version can be downloaded from the Qt website (see below)
* lib jpeg 8d/9a static MT. You should use the 9a version for OpenImageIO
* libpng 1.2.51/ 1.6.9 static MT, you should use version 1.6.9 for OpenImageIO and cairo
* OpenEXR 2.1 static MT
* OpenImageIO 1.4.8 static MT release
* libtiff 4.0.3 static MT release
* zlib 1.2.8 static MT release
* OpenColorIO 1.0.9 static MT release
* cairo 1.12.4 static MT release
* pixman 0.32.4 static MT release (64 bits only, it's easy to compile the 32 bits version yourself and it shouldn't be needed if 
you use the provided pre-built binary of cairo.).

###*Qt 5.3*

You'll need to install Qt libraries from [Qt download](http://qt-project.org/downloads). 
You probably want Qt 5.3.0 for Windows 32-bit (VS 2010, OpenGL, 593 MB)
Note that by default the binaries provided by Qt are 32 bits, which means you have to compile the 64 bits version yourself... or download
the pre-compiled binaries we provide (see above).

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

###Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/MrKepzie/OpenColorIO-Configs/archive/Natron-v2.0.tar.gz).
Place it at the root of Natron repository.


###Add the config.pri file

You have to define the locations of the required libraries.
This is done by creating a .pri file that will tell the .pro where to find those libraries.

- create the config.pri file next to the Project.pro file.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

*INCLUDEPATH* is the path to the include files
*LIBS* is the path to the libs

Here's an example of a config.pri file that supports both 32bit and 64bit builds:
---------------------------------------
64bit {

boost {
        INCLUDEPATH +=  $$quote(C:\\boost)
        CONFIG(release, debug|release): LIBS += -L$$quote(C:\\boost\\x64) -lboost_serialization-vc100-mt-1_57
		CONFIG(debug, debug|release):  LIBS += -L$$quote(C:\\boost\\x64) -lboost_serialization-vc100-mt-gd-1_57
}

glew{
    INCLUDEPATH +=  $$quote(C:\\glew\\include)
    LIBS += -L$$quote(C:\\glew\\lib\\Release\\x64) -lglew32
}

expat{
    INCLUDEPATH += $$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron\\libs\\OpenFX\\HostSupport\\expat-2.0.1\\lib)
    LIBS += -L$$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron\\libs\\OpenFX\\HostSupport\\expat-2.0.1\\x64\\bin\\Release) -llibexpatMT
    LIBS += shell32.lib
}

cairo {
	INCLUDEPATH += $$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron3rdParty\\cairo_1.12\\include)
	LIBS += -L$$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron3rdParty\\cairo_1.12\\lib\\x64) -lcairo
}

python {
	INCLUDEPATH += $$quote(C:\\Python34\\include)
	LIBS += -L$$quote(C:\\Python34\\libs) -lpython3
}

pyside {
	INCLUDEPATH += $$quote(C:\\Python34\\Lib\\site-packages\\PySide\\include\\PySide)
	INCLUDEPATH += $$quote(C:\\Python34\\Lib\\site-packages\\PySide\\include\\PySide\\QtGui)
	INCLUDEPATH += $$quote(C:\\Python34\\Lib\\site-packages\\PySide\\include\\PySide\\QtCore)
	INCLUDEPATH += $$quote(C:\\Qt\\4.8.6_win32\\include\\QtGui)
	LIBS += -L$$quote(C:\\Python34\\Lib\\site-packages\\PySide) -lpyside-python3.4
}

shiboken {
	INCLUDEPATH += $$quote(C:\\Python34\\Lib\\site-packages\\PySide\\include\\shiboken)
	LIBS += -L$$quote(C:\\Python34\\Lib\\site-packages\\PySide) -lshiboken-python3.4
}

}

32bit {


boost {
        INCLUDEPATH +=  $$quote(C:\\boost)
        CONFIG(release, debug|release): LIBS += -L$$quote(C:\\boost\\win32) -lboost_serialization-vc100-mt-1_57
		CONFIG(debug, debug|release): LIBS += -L$$quote(C:\\boost\\win32) -lboost_serialization-vc100-mt-gd-1_57
}

glew{
    INCLUDEPATH +=  $$quote(C:\\glew\\include)
    LIBS += -L$$quote(C:\\glew\\lib\\Release\\win32) -lglew32
}

expat{
    INCLUDEPATH += $$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron\\libs\\OpenFX\\HostSupport\\expat-2.0.1\\lib)
    LIBS += -L$$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron\\libs\\OpenFX\\HostSupport\\expat-2.0.1\\win32\\bin\\Release) -llibexpatMT
    LIBS += shell32.lib
}

cairo {
	INCLUDEPATH += $$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron3rdParty\\cairo_1.12\\include)
	LIBS += -L$$quote(C:\\Users\\lex\\Documents\\GitHub\\Natron3rdParty\\cairo_1.12\\lib\\win32) -lcairo
}

python {
	INCLUDEPATH += $$quote(C:\\Python34_win32\\include)
	LIBS += -L$$quote(C:\\Python34_win32\\libs) -lpython3
}

pyside {
	INCLUDEPATH += $$quote(C:\\Python34_win32\\Lib\\site-packages\\PySide\\include\\PySide)
	INCLUDEPATH += $$quote(C:\\Python34_win32\\Lib\\site-packages\\PySide\\include\\PySide\\QtGui)
	INCLUDEPATH += $$quote(C:\\Python34_win32\\Lib\\site-packages\\PySide\\include\\PySide\\QtCore)
	INCLUDEPATH += $$quote(C:\\Qt\\4.8.6_win32\\include\\QtGui)
	LIBS += -L$$quote(C:\\Python34_win32\\Lib\\site-packages\\PySide) -lpyside-python3.4
}

shiboken {
	INCLUDEPATH += $$quote(C:\\Python34_win32\\Lib\\site-packages\\PySide\\include\\shiboken)
	LIBS += -L$$quote(C:\\Python34_win32\\Lib\\site-packages\\PySide) -lshiboken-python3.4
}
}


---------------------------------------------

###*Copy the dll's over*

Copy all the required dll's to the executable directory. (Next to Natron.exe)
If Natron launches that means you got it all right;) Otherwise windows will prompt you
for the missing dll's when launching Natron.

###*Build*


	qmake -r -tp vc -spec win32-msvc2010 CONFIG+=64bit Project.pro -o Project64.sln

(adjust the qmake executable path to your system or add it to the path environment variable).

The vcproj "Natron" might complain of missing includes or linkage errors, if so adjust
the settings in the Additional include directories and Additional dependencies tab of
the property page of the project.

If you get the following linker error:
error LNK2019: unresolved external symbol WinMain referenced in function __tmainCRTStartup
Open the Natron project property pages. Go to Configuration Properties --> Linker --> Command Line.
In the Additional Options, add the following: 
     /ENTRY:"mainCRTStartup" 
(add a white space after the previous command)

In 64 bits mode (target x64), qmake doesn't set the target machine, it leaves the default value which is x86.
You'll have to set it manually in the properties of the HostSupport/Gui/Engine projects, as following:
Right click on the project, Configuration Properties,Librarian,General, Target Machine. Set it to Machinex64

	
### Generating Python bindings

This is not required as generated files are already in the repository. You would need to run it if you were to extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken for an explanation of the command line arguments.

shiboken  --avoid-protected-hack --enable-pyside-extensions --include-paths=..\Engine;..\Global;C:\Qt\4.8.6_win32\include;C:\Python34\Lib\site-packages\PySide\include\PySide --typesystem-paths=C:\Python34\Lib\site-packages\PySide\typesystems --output-directory=Engine Engine\Pyside_Engine_Python.h Engine\typesystem_engine.xml
shiboken  --avoid-protected-hack --enable-pyside-extensions --include-paths=..\Engine;..\Gui;..\Global;C:\Qt\4.8.6_win32\includeC;\Python34\Lib\site-packages\PySide\include\PySide  --typesystem-paths=C:\Python34\Lib\site-packages\PySide\typesystems;Engine --output-directory=Gui Gui\Pyside_Gui_Python.h  Gui\typesystem_natronGui.xml
### 32 bits vs 64 bits builds

Depending on the target architecture the dependencies are not the same. This can be achieved with the switch CONFIG+=64bit or CONFIG+=32bit
in the qmake call. 
Note that depending on the binary of qmake you're using to generate the visual studio solution file, it will set the Qt libraries in the
visual studio solution file to be the ones qmake was compiled with. In other words if you use the qmake downloaded from the qt website, then
probably this is 32 bits (you can check it with dumpbin /HEADERS C:\PATH\TO\qmake.exe , in the very first lines of the output: either x86 or x64).

So if you want to generate the 64 bits solution file with good dependencies make sure to make the qmake call with the 64 bits qmake binary
provided in the pre-built binaries.
In other words if I wanted to build both 32bits and 64bits version I would call:

	C:\PATH\TO\32BITS\QT\qmake -r -tp vc -spec win32-msvc2010 CONFIG+=32bit Project.pro -o Project32.sln
	C:\PATH\TO\64BITS\QT\qmake -r -tp vc -spec win32-msvc2010 CONFIG+=64bit Project.pro -o Project64.sln

###Build from the command line with MSBuild

	MSBuild Project32.sln /p:Configuration=Release;Platform=win32 /t:Natron /m
	MSBuild Project64.sln /p:Configuration=Release;Platform=x64 /t:Natron /m

