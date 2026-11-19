.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Windows
=======

This chapter explains how to install and maintain Natron on Windows.

Requirements
------------

Natron needs 64-bit Windows 10 or later on an x86-64 processor, and a graphics
card with OpenGL 2.0. See :ref:`system-requirements` for the details.

If your graphics driver does not provide the OpenGL features Natron needs, use
the software OpenGL renderer: select the *OpenGL Software Renderer (FOR LEGACY
HARDWARE ONLY)* component in the installer, or, if you run Natron without
installing it, copy *bin\\mesa\\opengl32.dll* to *bin\\*. Software rendering
is much slower.

Download
--------

Go to https://natrongithub.github.io/#download and download the latest version
for Windows: a ZIP file, ``Natron-<version>-Windows-x86_64.zip``. Extract it
(right-click it in the File Explorer and select *Extract All…*). The extracted
folder contains Natron, ready to run, and its installer, ``Setup.exe``.

Install
-------

Double-click ``Setup.exe`` in the extracted folder: the installer copies Natron
from that folder, so keep the folder until the installation is complete.

The installer first asks whether to install Natron for all users (this needs
administrator rights) or only for you. It then shows the license: Natron and
its plug-ins are licensed under the GNU GPL version 2, and the licenses of all
the components included in Natron are listed in Help → About after
installation. The next pages let you choose:

* the installation folder: the default, in *Program Files* (or in your user
  folder for a single-user installation), is recommended;
* the components to install. Natron and the OpenFX IO and Misc plug-ins are
  always installed; the PyPlugs, the OpenColorIO configurations and the CImg,
  Shadertoy, Arena and G'MIC plug-ins are selected by default. Keep the
  *OpenGL Software Renderer (FOR LEGACY HARDWARE ONLY)* component unselected
  unless your graphics driver does not work with Natron (see above);
* the Start menu folder, and whether to create a desktop icon.

The installer also associates Natron project files (``.ntp``) with Natron. At
the end, it can start Natron. Afterwards, start Natron from the Start menu (or
the desktop icon). You can delete the ZIP file and the extracted folder.

Update and uninstall
--------------------

To update Natron, install the new version the same way: the installer offers
the folder of the previous installation.
To uninstall Natron, use *Settings → Apps → Installed apps* (*Apps & features*
on Windows 10), or the *Uninstall Natron* shortcut in its Start menu folder.

Run without installing
----------------------

Natron also runs directly from the extracted folder, without installing it:
double-click *bin\\Natron.exe*. This is useful to try a new version next to
the installed one, or on a computer where you cannot install software. To
remove this copy, delete the folder.
