To have a working project using QtCreator please make sure everything is set as follow :

WARNING : The QtCreator project files currently only works for linux , you should use the Xcode based project on mac osx or the Visual Studio 2010 solution file for windows.

On linux, not all libraries are provided, you'll find only OpenExr_linux under the /libs folder at the root of the Powiter_Core

In order to have Powiter working with QtCreator you should install these libraries with the developer files :

- ffmpeg
- freetype2
- ftgl
- Qt (4.8)

Once you installed those libraries , open the PowiterWrapper.pro file in Powiter_Core/ProjectFiles/PowiterLinux/PowiterWrapper.
Expand the PowiterLib subdir , and open PowiterLib.pro . 

At the line INCLUDEPATH += /usr/local/Trolltech/Qt-4.8.4/include  ,  change the path to your installation of the Qt headers.

Also if you installed freetype2 in some weird place , you might want to change 
INCLUDEPATH += /usr/include/freetype2

Same point applies to ftgl.
Again for libav*/libswscale : if they do not reside in /usr/lib/i386-linux-gnu change the corresponding lines in the .pro.

Now you'll need to change the build directory of PowiterWrapper:
Go to the "Projects" tab : in the toolbar click the PowiterWrapper tab if you're not already in this tab. Now click the Build tab of the Desktop widget and change the Build directory to:

 YOUR_PATH_TO_POWITER/Powiter_Core/ProjectFiles/PowiterLinux/build

Now in the Run tab of the desktop widget, change the working directory to :

 YOUR_PATH_TO_POWITER/Powiter_Core

Once you've done that you should be able to do right-click on PowiterWrapper and build.
This is not done yet you cannot run the Powiter project because it will not run with a proper working directory ( QtCreator is not really helpful for this).
-Open the Powiter.pro file located under Powiter_Core/ProjectFiles/PowiterLinux/PowiterWrapper/Powiter
-QtCreator will normally ask to configure the project: 
specify Powiter_Core/ProjectFiles/PowiterLinux/build  as the build directory for both Release/Debug versions.
Now that it is opened, go to the projects tab, click the Powiter tab and select the run tab from the desktop widget. Change the working directory to:

 YOUR_PATH_TO_POWITER/Powiter_Core

That's it now you can run powiter (right click, run) from the "Powiter" project (NOT the one under PowiterWrapper/) and if you need to make modification to either Powiter or PowiterLib, make them under PowiterWrapper, and build (right click, build) using PowiterWrapper.




