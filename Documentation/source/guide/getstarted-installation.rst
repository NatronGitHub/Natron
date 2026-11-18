.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Installation
============

This chapter explains how to install Natron on Windows, macOS and Linux.

.. _system-requirements:

System requirements
-------------------

Natron 2.6 binaries are 64-bit only. 32-bit systems are no longer supported.

Operating system and processor:

- **Windows:** Windows 10 or later, 64-bit, on an x86-64 processor.
- **macOS:** macOS 14 (Sonoma) or later, on a Mac with Apple Silicon (arm64).
  The 2.6 binaries do not run on Intel Macs: use Natron 2.5 there, or build
  Natron from source.
- **Linux:** a 64-bit distribution (x86-64 or aarch64) with glibc 2.28 or
  later. See :doc:`getstarted-installation-linux` for details.

Graphics card:

- The viewer needs OpenGL 2.0 with the ``GL_ARB_vertex_buffer_object`` and
  ``GL_ARB_pixel_buffer_object`` extensions. Almost every graphics card and
  integrated GPU made in the last fifteen years has them, if its driver is
  installed.
- GPU rendering, which a few plug-ins such as
  :ref:`Shadertoy <net.sf.openfx.Shadertoy>` can use, also needs
  ``GL_ARB_texture_float``, ``GL_ARB_framebuffer_object`` (or
  ``GL_EXT_framebuffer_object``) and ``GL_ARB_vertex_array_object`` (or
  ``GL_APPLE_vertex_array_object``). Without them, these plug-ins render on
  the CPU.
- If the graphics driver does not meet these requirements, Natron can use a
  slower software OpenGL renderer: see :ref:`the FAQ <software-opengl>`.

Memory: Natron keeps rendered images in a RAM cache, which makes playback and
interaction fast. More RAM means a larger cache, and large images or long
sequences need several gigabytes. The cache size is set in Edit → Preferences →
Caching.

.. toctree::
   :maxdepth: 2

   getstarted-installation-windows
   getstarted-installation-mac
   getstarted-installation-linux

Additional elements
-------------------

Community scripts
~~~~~~~~~~~~~~~~~

The `natron-python-scripting <https://github.com/NatronGitHub/natron-python-scripting>`_
repository holds community Python scripts that add tools, mostly to the Tools
and Edit menus: predefined Roto shapes, multi-layer EXR extraction, node
connection tools, and more. Several of them make Natron behave more like Nuke.

To install them:

- Copy the contents of the repository into the ``.Natron`` folder in your home
  directory (see :ref:`natronPath`).
- Restart Natron.

These scripts are experimental.

Community plug-ins
~~~~~~~~~~~~~~~~~~

The `natron-plugins <https://github.com/NatronGitHub/natron-plugins>`_
repository holds community Python plug-ins (PyPlugs): animated textures for
motion design, and equivalents of common Nuke gizmos (DespillMadness,
PushPixel, …).

To install them:

- Copy the contents of the repository into a folder of your choice.
- In Edit → Preferences → Plug-ins, add that folder to "PyPlugs search path".
- Click "Save" and restart Natron.

The new tools appear in the toolbar on the left. These plug-ins are experimental.
