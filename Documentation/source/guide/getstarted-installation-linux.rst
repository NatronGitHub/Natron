.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Linux
=====

This chapter explains how to install and maintain Natron on Linux.

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

If your graphics driver does not provide the OpenGL features Natron needs, run
Natron with Mesa's software renderer: set the environment variable
``LIBGL_ALWAYS_SOFTWARE=1`` before starting Natron. Software rendering is much
slower.

Download
--------

Go to https://natrongithub.github.io/#download and download the latest version
for Linux. Natron for Linux is a portable archive,
``Natron-<version>-Linux-<arch>-no-installer.tar.xz``, where ``<arch>`` is
``x86_64`` or ``aarch64``: there is no installer, and no RPM or DEB package.

Install and run
---------------

Extract the archive anywhere, for example in your home directory: in most file
managers, right-click it and select 'Extract Here', or in a terminal:

.. code-block:: bash

   tar xf Natron-<version>-Linux-x86_64-no-installer.tar.xz

This creates a folder of the same name. Start Natron by running the ``Natron``
script at the top of that folder (``NatronRenderer``, next to it, renders from
the command line). Don't run the programs in its ``bin`` folder directly: the
scripts set up the environment that they need.

*If the script does not start, make it executable: right-click it, select
Properties, and tick 'Allow executing file as program' (the name of this option
depends on the desktop environment). In a terminal, you can run*
``chmod +x Natron NatronRenderer`` *instead.*

Natron and its plug-ins are licensed under the GNU GPL version 2. The licenses
of all the components included in Natron are listed in Help → About.

To update Natron, download and extract the new version (you can keep the old
one, for example to open old projects). To uninstall it, delete the folder.

Other ways to install Natron
----------------------------

Natron is also available on `Flathub <https://flathub.org/apps/fr.natron.Natron>`_
(``flatpak install flathub fr.natron.Natron``). This package is maintained
separately, and may be older than the latest release.
