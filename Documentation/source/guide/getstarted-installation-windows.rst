.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Windows
=======

This chapter explains how to install, update and uninstall Natron on Windows.

Requirements
------------

Natron needs 64-bit Windows 10 or later on an x86-64 processor, and a graphics
card with OpenGL 2.0. See :ref:`system-requirements` for the details.

Download
--------

Go to https://natrongithub.github.io/#download and download the latest version
for Windows. It is a ZIP file, ``Natron-<version>-Windows-x86_64.zip``.

Extract the ZIP file: right-click it in File Explorer and select *Extract
All…*. This creates a folder that holds another folder with the same name,
``Natron-<version>-Windows-x86_64``. That inner folder holds a copy of Natron
that is ready to run, and its installer, ``Setup.exe``.

Always extract the ZIP file first. Don't start ``Setup.exe`` from inside the
ZIP file: the installer copies Natron from the folder next to it.

Windows may show a *Windows protected your PC* warning when you start
``Setup.exe`` or Natron for the first time, because the Natron programs are
not signed. Click *More info*, then *Run anyway*.

Install
-------

Double-click ``Setup.exe`` in the extracted folder. The installer asks a few
questions:

#. **For all users or only for you.** Installing for all users needs
   administrator rights, and Natron goes to ``C:\Program Files\Natron``.
   Installing only for you puts Natron in your user folder
   (``%LOCALAPPDATA%\Programs\Natron``).
#. **The license.** Natron and its plug-ins are licensed under the GNU GPL
   version 2 or later. Help → About lists the licenses of all the components
   included in Natron.
#. **The installation folder.** Keep the default.
#. **The components.** Natron and the OpenFX IO and Misc plug-ins are always
   installed. The PyPlugs, the OpenColorIO configurations, and the CImg,
   Shadertoy, Arena and G'MIC plug-ins are selected by default. Keep
   *OpenColorIO Configs* selected: Natron's default color management
   configuration is in it. Leave *OpenGL Software Renderer (FOR LEGACY
   HARDWARE ONLY)* unselected, unless your graphics driver doesn't work with
   Natron (see `Software OpenGL`_).
#. **The Start menu folder,** and whether to add a desktop icon.

The installer also makes Natron open Natron project files (``.ntp``). At the
end, it can start Natron. Later, start Natron from the Start menu, or from the
desktop icon.

When the installation is complete, you can delete the ZIP file and the
extracted folder.

Run without installing
----------------------

Natron also runs straight from the extracted folder: double-click
``bin\Natron.exe``. Use this to try a new version next to the installed one,
or on a computer where you can't install software. This copy adds no Start
menu entry, and double-clicking a ``.ntp`` file doesn't start it. It uses the
same settings as the installed Natron (see `Your files`_). To remove it,
delete the folder.

Your files
----------

Natron keeps your files outside its installation folder, and all the copies of
Natron that you run share them:

* the settings, in the registry key ``HKEY_CURRENT_USER\Software\INRIA\Natron``;
* the disk cache, in ``%LOCALAPPDATA%\INRIA\Natron\cache``, unless you chose
  another folder in Edit → Preferences → Caching;
* the auto-saved projects, in ``%LOCALAPPDATA%\INRIA\Natron\Autosaves``;
* your Python scripts and PyPlugs, in ``C:\Users\<username>\.Natron`` (see
  :ref:`natronPath`).

Keep the OpenFX plug-ins that you add yourself outside the Natron folder: put
them in ``C:\Program Files\Common Files\OFX\Plugins``, or add their folder in
Edit → Preferences → Plug-ins → *OpenFX plug-ins search path*.

Update
------

To update Natron, run ``Setup.exe`` of the new version in the same way. If
Natron is already installed, the installer doesn't ask for a folder: it
installs into the folder of the current installation and replaces Natron's
files. It remembers one folder for "all users" and one for "only for you": if
you choose the other mode, you get a second installation. The new version
uses the settings of the old one.

Natron 2.5 and older used another installer, which put each version in its own
folder (for example ``C:\Program Files\INRIA\Natron-2.5.0``). These versions
stay installed next to Natron 2.6. Uninstall them from *Settings → Apps* when
you no longer need them. Don't install Natron 2.6 into one of their folders.

Uninstall
---------

To uninstall Natron, open *Settings → Apps → Installed apps* (*Apps &
features* on Windows 10), select Natron, and select *Uninstall*. You can also
use the *Uninstall Natron* shortcut in the Natron folder of the Start menu.
Then delete the Natron folder, if it is still there.

Uninstalling doesn't remove your files (see `Your files`_): delete them too
if you don't need them any more.

Software OpenGL
---------------

If your graphics driver doesn't provide the OpenGL features that Natron needs,
use the software OpenGL renderer that comes with Natron. It is much slower
than a graphics card.

- If you installed Natron: start ``Setup.exe`` from the extracted folder again
  (extract the ZIP file again if you deleted it), and select the *OpenGL
  Software Renderer (FOR LEGACY HARDWARE ONLY)* component.
- If you run Natron without installing it: copy ``bin\mesa\opengl32.dll`` to
  ``bin\``.

To use the graphics card again, run ``Setup.exe`` again without that
component. Without installing, delete ``bin\opengl32.dll``.
