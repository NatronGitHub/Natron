.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
FAQ
===

.. toctree::
   :maxdepth: 2


Can I use Natron for commercial work?
*************************************

Yes. Anything you create with Natron is yours and you’re free to do anything you want with it.


What operating systems are supported by Natron?
***********************************************

Natron officially supports:

* Windows 7, 8 and 10 with latest service packs.
* MacOSX 10.6 or greater
* Linux 2.6.18 or greater (Glibc 2.12+/libgcc 4.4+)


Why did you make Natron free of charge?
***************************************

Our original motives were to create a tool for people who needed it and that may felt left-aside by the software editors pricing plans, that is:

* Students who want to learn compositing at home
* Schools that may not be able to buy expensive software licenses

Another reason why Natron was developped mainly at `INRIA <http://www.inria.fr/en>`_ is because a compositing software is a playground that enables scientists in computer vision/graphics to develop, test exchange and publish results easily  on such platform.

One great mission of a free open-source software is to aim to create common practises so everyone can benefit of it.

On the other hand, being free of charge, Natron can be installed on large-scale render farms without wondering about licensing issues.


What is OpenFX?
***************

`OpenFX <http://openeffects.org/>`_ is a standard for creating visual effects plug-ins for compositing and editor applications.

As of today several applications are compatible with this plug-in format: (meaning you can use the same plug-ins in all of them)

* Nuke 5.1+, by The Foundry
* Vegas 10+, by Sony
* SCRATCH 6.1+, by Assimilate
* Fusion 5.1+, by Blackmagic Design (formerly by eyeon)
* DaVinci Resolve 10+, by Blackmagic Design
* DustBuster+ 4.5+, by HS-ART
* Baselight 2.2+ by FilmLight
* Nucoda Film Master 2011.2.058+
* SGO Mistika 6.5.35+
* Autodesk Toxik 2009+
* Avid DS 10.3+
* Natron
* ButtleOFX
* TuttleOFX

Can I use commercial and proprietary plug-ins within Natron?
************************************************************

Yes. Natron doesn’t limit you to open-source plug-ins.


Is my graphics card supported?
******************************

An OpenGL 2.0 compatible graphics card is needed to run Natron (2.1+) with hardware-accelerated rendering. Other graphics cards work with software-only rendering (see below).

The following graphics cards are supported for hardware-accelerated rendering:

 * Intel GMA 3150 (Linux-only)
 * Intel GMA X3xxx (Linux-only)
 * Intel GMA X4xxx (Windows 7 & Linux)
 * Intel HD (Ironlake) (Windows 7 & Linux)
 * Intel HD 2000/3000 (Sandy Bridge) (Windows 7/Linux/Mac)
 * Intel HD 4000 and greater (All platforms)
 * Nvidia GeForce 6 series and greater
 * Nvidia Quadro FX and greater
 * Nvidia Quadro NVS 285 and greater
 * ATI/AMD Radeon R300 and greater
 * ATI/AMD FireGL T2-64 and greater (FirePro)

Cards not listed here will probably not support hardware-accelerated rendering.

On Windows and Linux you can enable software rendering. On Linux, enable the environment variable LIBGL_ALWAYS_SOFTWARE=1 before running Natron. On Windows, enable the legacy hardware package in the installer.
