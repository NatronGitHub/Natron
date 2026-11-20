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

Natron and its plug-ins are licensed under the GNU GPL version 2. Help → About
lists the licenses of all the components included in Natron.

Add Natron to the desktop
-------------------------

The archive doesn't change anything outside its folder. To add Natron to the
applications menu, with its icons, and to open Natron project files
(``.ntp``) with Natron, run ``natron-mime.sh`` from the ``bin`` folder:

.. code-block:: bash

   Natron-<version>-Linux-x86_64-no-installer/bin/natron-mime.sh

Run it as your normal user: it registers Natron for you only, in
``~/.local/share``. Run as root, it registers Natron for all users, in
``/usr/share``.

The menu entry starts the copy of Natron that registered it. If you move or
update Natron, run ``natron-mime.sh`` from the new folder again.

Update
------

To update Natron, download and extract the new version. You can keep the old
version next to it, for example to open old projects. If you added Natron to
the desktop, run ``natron-mime.sh`` from the new version.

Uninstall
---------

To uninstall Natron, delete its folder. Your settings and your projects are
not in that folder, so they stay.

If you ran ``natron-mime.sh``, also remove the Natron menu entry, icons and
file type from ``~/.local/share`` (or ``/usr/share``, if you ran it as root):
the Natron files in its ``applications``, ``icons`` and ``mime/packages``
folders. Then update the desktop databases:

.. code-block:: bash

   update-desktop-database ~/.local/share/applications
   update-mime-database ~/.local/share/mime

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
