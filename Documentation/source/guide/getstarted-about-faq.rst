.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
FAQ
===

Can I use Natron for commercial work?
*************************************

Yes. Anything you create with Natron is yours, and you are free to do anything you want with it.


What operating systems are supported by Natron?
***********************************************

Natron 2.6 runs on:

* Windows 10 or later, 64-bit (x86-64);
* macOS 14 (Sonoma) or later, on Apple Silicon (arm64) Macs;
* 64-bit Linux (x86-64 or aarch64) with glibc 2.28 or later, for example RHEL, Rocky Linux or AlmaLinux 8, Debian 12, Ubuntu 22.04 LTS, or later versions.

32-bit systems and Intel Macs are not supported by Natron 2.6. See :ref:`system-requirements` for the details.


Why did you make Natron free of charge?
***************************************

Natron was created for people who could not afford the commercial compositing software:

* students who want to learn compositing at home;
* schools that cannot buy expensive software licenses.

Natron was first developed at `Inria <https://www.inria.fr/en>`_, because a compositing application is also a platform where researchers in computer vision and computer graphics can develop, test, exchange and publish their results.

Being free of charge, Natron can also be installed on large render farms without license costs.


What is OpenFX?
***************

`OpenFX <https://openeffects.org/>`_ is an open standard for visual effects plug-ins, maintained by the Academy Software Foundation. A plug-in written for OpenFX works in every application that supports the standard, for example Nuke, DaVinci Resolve, Fusion, VEGAS Pro, Baselight, Flame, SCRATCH, and Natron.

All the nodes bundled with Natron, except a few built-in ones such as Roto, Tracker or the Viewer, are OpenFX plug-ins.

Can I use commercial and proprietary plug-ins within Natron?
************************************************************

Yes. Natron can load any OpenFX plug-in, open-source or not. Commercial plug-ins are mostly tested by their vendors in other hosts, so some of them may not work fully in Natron: try the demo version first.


Is my graphics card supported?
******************************

Natron needs a graphics card with OpenGL 2.0 and a few extensions (see :ref:`system-requirements`). Almost any graphics card or integrated GPU made in the last fifteen years works, provided its OpenGL driver is installed.

.. _software-opengl:

If your graphics card or driver is not supported, Natron can use a software OpenGL renderer. It is much slower.

* On Linux, set the environment variable ``LIBGL_ALWAYS_SOFTWARE=1`` before starting Natron. This uses Mesa's software renderer.
* On Windows, select the *Legacy hardware support* package in the installer or, with the portable ZIP file, copy ``bin\mesa\opengl32.dll`` to ``bin\``.
