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
for Windows. It is a portable ZIP file, ``Natron-<version>-Windows-x86_64.zip``.
There is no installer: you unzip Natron and run it where it is.

Install
-------

#. Extract the ZIP file: right-click it in File Explorer and select *Extract
   All…*. This creates a folder that holds another folder with the same name,
   ``Natron-<version>-Windows-x86_64``: that inner folder is Natron.
#. Move that inner folder where you want to keep Natron, for example to
   ``C:\Users\<username>\Natron`` (no administrator rights needed) or
   ``C:\Program Files\Natron``. Always extract the ZIP file first: Natron
   doesn't run from inside it.

   If *Extract All* reports that a path is too long, extract to a short
   folder instead, such as ``C:\Natron`` (choose it in the *Extract All*
   dialog): by default, the tree is nested twice under the long name of the
   ZIP file, and Windows limits paths to 260 characters.
#. Start Natron: double-click ``bin\Natron.exe`` in that folder.

Windows may show a *Windows protected your PC* warning the first time you start
Natron, because the Natron programs are not signed. Click *More info*, then
*Run anyway*.

Natron and its plug-ins are licensed under the GNU GPL version 2 or later.
Help → About lists the licenses of all the components included in Natron.

Everything Natron needs is in its folder: its plug-ins, Python, the PyPlugs and
the OpenColorIO configurations. Nothing is written to the registry or to
``PATH`` when you unzip it, so you can keep several versions side by side,
each in its own folder.

Start menu, desktop and ``.ntp`` files
--------------------------------------

Because there is no installer, unzipping Natron adds no Start menu entry, no
desktop icon and no uninstall entry, and double-clicking a Natron project
file (``.ntp``) doesn't start Natron. Project files also keep the generic file
icon, even after you make them open with Natron, and you can't leave out a set
of plug-ins (all the bundled OpenFX plug-ins, PyPlugs and OpenColorIO
configurations are always there). To set up the rest by hand:

- **Start menu or taskbar:** right-click ``bin\Natron.exe``, then select *Pin
  to Start* or *Pin to taskbar* (on Windows 11, first *Show more options*).
- **Desktop icon:** right-click ``bin\Natron.exe``, select *Send to → Desktop
  (create shortcut)* (on Windows 11, first *Show more options*).
- **Open ``.ntp`` files with Natron:** right-click a ``.ntp`` file, select
  *Open with → Choose another app*, then *Choose an app on your PC* (or *More
  apps → Look for another app on this PC*), select ``bin\Natron.exe``, and
  select *Always*.

If you move the Natron folder, or replace it with a new version in another
folder, do these again.

Your files
----------

Natron keeps your files outside its folder, and all the copies of Natron that
you run share them:

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

To update Natron, extract the ZIP file of the new version, and either keep it
next to the old one or delete the old Natron folder and put the new one in its
place (quit Natron first). Don't extract a new version over an old one: files
of the old version would stay behind. The new version uses the settings of the
old one. If you pinned Natron or made ``.ntp`` files open with it, and the new
version is in another folder, set these up again (see `Start menu, desktop and
.ntp files`_).

Earlier Natron 2.6 test builds came with an installer, ``Setup.exe``. If you
installed one of them, uninstall it from *Settings → Apps → Installed apps*
(*Apps & features* on Windows 10). Natron 2.5 and older
used another installer, which put each version in its own folder (for example
``C:\Program Files\INRIA\Natron-2.5.0``); these versions can stay installed
next to Natron 2.6. To remove one, select the Natron published by INRIA in
*Installed apps* and select *Uninstall*. Until you remove them, double-clicking
a ``.ntp`` file may still open the older version: make ``.ntp`` files open with
the new Natron as described above.

Uninstall
---------

To uninstall Natron, quit it and delete its folder, then remove any shortcut
you made to it.

Deleting the folder doesn't remove your files (see `Your files`_): delete them
too if you don't need them any more.

Software OpenGL
---------------

If your graphics driver doesn't provide the OpenGL features that Natron needs,
use the software OpenGL renderer that comes with Natron. It is much slower
than a graphics card. Copy ``bin\mesa\opengl32.dll`` to ``bin\`` in the Natron
folder. To use the graphics card again, delete ``bin\opengl32.dll``.
