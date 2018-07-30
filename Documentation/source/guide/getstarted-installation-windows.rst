.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Windows
=======

.. toctree::
   :maxdepth: 2

This chapter will guide your through the installation and maintenance of Natron on Windows.

Requirements
------------

Natron will work on Windows 7, 8.x, 10 with latest updates.

The basic requirements are:

 * x86 compatible CPU (Core2 x86_64 or higher recommended)
 * OpenGL 2.0 or higher with the following extensions:
    * **GL_ARB_texture_non_power_of_two** *(Viewer and OpenGL rendering)*
    * **GL_ARB_shader_objects** *(Viewer and OpenGL rendering)*
    * **GL_ARB_vertex_buffer_object** *(Viewer and OpenGL rendering)*
    * **GL_ARB_pixel_buffer_object** *(Viewer and OpenGL rendering)*
    * **GL_ARB_vertex_array_object** or **GL_APPLE_vertex_array_object** *(OpenGL rendering only)*
    * **GL_ARB_framebuffer_object** or **GL_EXT_framebuffer_object** *(OpenGL rendering only)*
    * **GL_ARB_texture_float** *(OpenGL rendering only)*

If you don't have the minimum required OpenGL extensions we provide a Software OpenGL solution, install the package *Software OpenGL* from the installer. If you have the portable ZIP file copy *bin\\mesa\\opengl32.dll* to *bin\\*.

Download
--------

Navigate to https://natrongithub.github.io/#download and download the latest version. This documentation will assume that you downloaded the installer (our default and recommended choice).

.. image:: _images/win_install_01.png
   :width: 40%

Install
-------

You are now ready to start the installation, double-click on the setup file to start the installation.

.. image:: _images/win_install_02.png
   :width: 15%

You should now be greated with the installation wizard.

.. image:: _images/win_install_03.png
   :width: 40%

Click 'Next' to start the installation, you first option is where to install Natron. The default location is recommended.

.. image:: _images/win_install_04.png
   :width: 40%

Your next option is the package selection, most users should accept the default. Each package has an more in-depth description if you want to know what they provide.

.. image:: _images/win_install_05.png
   :width: 40%

Then comes the standard license agreement, Natron and it's plug-ins are licensed under the GPL version 2. You can read more about the licenses for each component included in Natron after installation (in menu Help→About).

.. image:: _images/win_install_06.png
   :width: 40%

The installation wizard is now ready to install Natron on your computer. The process should not take more than a minute or two (depending on your computer).

.. image:: _images/win_install_07.png
   :width: 40%

The installation is now over! Start Natron and enjoy.

.. image:: _images/win_install_08.png
   :width: 40%

Natron can now be started from the desktop icon or from the start menu.

.. image:: _images/win_install_09.png
   :width: 40%

Maintenance
-----------

Natron includes a maintenance tool called 'NatronSetup', with this application you can easily upgrade Natron and it's components when a new version is available. You can also add or remove individual packages, or remove Natron completely. The application should be in the same start menu folder as Natron, or you can start it from the folder where you installed Natron.

.. image:: _images/win_install_10.png
   :width: 40%
.. image:: _images/win_install_11.png
   :width: 40%

The application also include a basic settings category, where you can configure proxy and other advanced options.
