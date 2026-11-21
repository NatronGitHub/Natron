.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Linux
=====

This chapter explains how to install, update and uninstall Natron on Linux.

Requirements
------------

The Linux binaries are built on Rocky Linux 9, for x86-64 and aarch64
processors. They run on any 64-bit Linux distribution that has:

* glibc 2.34 or later;
* the C++ and OpenMP runtime libraries of GCC 11 or later (``libstdc++``,
  ``libgcc_s`` and ``libgomp``), which almost every distribution installs by
  default;
* the X11 client libraries (libX11, libxcb, libxkbcommon), at least as recent
  as those of Rocky Linux 9. Natron is an X11 application: on a Wayland desktop
  it runs through XWayland;
* an OpenGL driver (``libGL``): see :ref:`system-requirements`.

Natron includes its other libraries (Qt, Python, fontconfig, FreeType, …), but
not these system libraries.

For example, these distributions meet the requirements:

* `RHEL <https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux>`_,
  `Rocky Linux <https://rockylinux.org/>`_ and `AlmaLinux <https://almalinux.org/>`_ 9 and later
* `Debian <https://www.debian.org/>`_ 12 "bookworm" and later
* `Ubuntu <https://ubuntu.com/>`_ 22.04 LTS and later
* `Fedora <https://fedoraproject.org/>`_ (all supported releases)

Download
--------

Go to https://natrongithub.github.io/#download and download the latest version
for Linux. It is a portable archive,
``Natron-<version>-Linux-<arch>-no-installer.tar.xz``, where ``<arch>`` is
``x86_64`` or ``aarch64``. There is no installer, and no RPM or DEB package.

Install and run
---------------

Extract the archive anywhere, for example in your home directory. In most file
managers, right-click it and select *Extract Here*. In a terminal, run:

.. code-block:: bash

   tar xf Natron-<version>-Linux-x86_64-no-installer.tar.xz

This creates a folder with the same name. Start Natron with the ``Natron``
script at the top of that folder. ``NatronRenderer``, next to it, renders
projects from the command line (see :doc:`../devel/commandline`). Don't
start the programs in the ``bin`` folder directly: the two scripts set up the
environment that they need.

If the scripts don't start, make them executable. In the file manager,
right-click each one, select *Properties*, and turn on *Allow executing file
as program* (the name of this option depends on the desktop). In a terminal,
run ``chmod +x Natron NatronRenderer`` instead.

Natron and its plug-ins are licensed under the GNU GPL version 2 or later.
Help → About lists the licenses of all the components included in Natron.

Add Natron to the desktop
-------------------------

The archive doesn't change anything outside its folder. To add Natron to the
applications menu, and to open Natron project files (``.ntp``) with a
double-click, run ``natron-mime.sh`` from the ``bin`` folder:

.. code-block:: bash

   Natron-<version>-Linux-x86_64-no-installer/bin/natron-mime.sh

Run it as your normal user to add Natron for you only, in
``~/.local/share``. Run it as root, for example with ``sudo``, to add it for
all users, in ``/usr/local/share``.

It adds a menu entry that starts this copy of Natron, the Natron icons, and
the Natron project file type. Run it again after you move or update Natron:
the menu entry then starts the copy that you ran it from. You may have to log
out and log in again before the menu shows Natron.

Your files
----------

Natron keeps your files outside its folder, and all the copies of Natron that
you run share them:

* the settings, in ``~/.config/INRIA/Natron.conf``;
* the disk cache, in ``~/.cache/INRIA/Natron``, unless you chose another
  folder in Edit → Preferences → Caching;
* the auto-saved projects, in ``~/.local/share/INRIA/Natron/Autosaves``;
* your Python scripts and PyPlugs, in ``~/.Natron`` (see :ref:`natronPath`).

Update
------

To update Natron, download and extract the new version. You can keep the old
version next to it, for example to open old projects. The new version uses the
settings of the old one.

Keep the OpenFX plug-ins that you add yourself outside the Natron folder, so
that they stay when you update: put them in ``/usr/OFX/Plugins``, or add their
folder in *Edit* → *Preferences* → *Plug-ins* → *OpenFX plug-ins search path*.

Uninstall
---------

If you added Natron to the desktop, remove it from the desktop first. Run
``natron-mime.sh --uninstall`` as the same user as when you added it (your
normal user, or root):

.. code-block:: bash

   Natron-<version>-Linux-x86_64-no-installer/bin/natron-mime.sh --uninstall

Then delete the Natron folder. This doesn't remove your files (see
`Your files`_): delete them too if you don't need them any more.

Software OpenGL
---------------

If your graphics driver doesn't provide the OpenGL features that Natron needs,
use Mesa's software renderer. Set the environment variable
``LIBGL_ALWAYS_SOFTWARE=1`` when you start Natron:

.. code-block:: bash

   LIBGL_ALWAYS_SOFTWARE=1 ./Natron

This needs the Mesa OpenGL drivers of your distribution, which most
distributions install with the desktop. Software rendering is much slower than
a graphics card.

Other ways to install Natron
----------------------------

Natron is also available on `Flathub <https://flathub.org/apps/fr.natron.Natron>`_
(``flatpak install flathub fr.natron.Natron``). This package is maintained
separately, and may be older than the latest release.
