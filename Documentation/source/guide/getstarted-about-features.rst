.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Features
========

* **32-bit floating-point linear color pipeline.**
* **Color management** with `OpenColorIO <https://opencolorio.org/>`_.
* **Dozens of image and video formats:** EXR, DPX, TIFF, PSD, SVG, camera raw, JPEG, PNG, and video formats such as H.264, ProRes and DNxHR, through OpenImageIO, FFmpeg and other libraries.
* **Free and open-source OpenFX plug-ins.** These are included in the Natron binaries:

  * `openfx-io <https://github.com/NatronGitHub/openfx-io>`_: readers and writers;
  * `openfx-misc <https://github.com/NatronGitHub/openfx-misc>`_: color, filter, keying, merge, transform and other basic nodes;
  * `openfx-arena <https://github.com/NatronGitHub/openfx-arena>`_: text, PSD, SVG and other extra nodes;
  * `openfx-gmic <https://github.com/NatronGitHub/openfx-gmic>`_: G'MIC filters.

  Other OpenFX plug-ins can be installed: add their folder to "OpenFX plug-ins search path" in Edit → Preferences → Plug-ins.

* **Commercial OpenFX plug-ins**, for example from `RE:Vision Effects <https://revisionfx.com/>`_, `Boris FX <https://borisfx.com/>`_ (Sapphire) or `Neat Video <https://www.neatvideo.com/>`_. See the FAQ about :doc:`commercial plug-ins <getstarted-about-faq>`.
* **OpenFX support:** Natron implements the OpenFX 1.4 host API, with the Natron and Nuke extensions. It is built with the OpenFX 1.5.1 headers, but the features added in OpenFX 1.5 (string choice parameters, the Draw suite, OpenFX color management, CUDA, Metal and OpenCL rendering) are not supported yet.
* **Familiar user interface:** the interface follows the conventions of other node-based compositors. Its panes can be rearranged and spread over several screens, and layouts can be saved and reused.
* **Fast feedback:** the multi-threaded rendering pipeline and proxy rendering (computing at a lower resolution) keep the viewer responsive.
* **Multi-tasking:** Natron can render several graphs at the same time, and use all the cores of the CPU.
* **Command-line rendering:** NatronRenderer runs projects and Python scripts without a display, for example over ssh or on a render farm managed by `Afanasy <https://cgru.info/>`_.
* **Fast and interactive viewer:** smooth zooming and panning, even on very large images (tested on 27k × 30k images).
* **Real-time playback:** rendered frames are kept in a RAM and disk cache, so they play back instantly.
* **Animation:** a curve editor and a dope sheet to edit keyframes, and expressions on parameters.
* **Multi-view workflow:** all the views (for example the left and right views of a stereo project) are processed in the same graph. The :ref:`OneView <fr.inria.built-in.OneView>` node extracts one view.
* **Rotoscoping and rotopainting:** draw and animate masks and paint strokes with the :ref:`Roto <fr.inria.built-in.Roto>` and :ref:`RotoPaint <fr.inria.built-in.RotoPaint>` nodes.
* **Tracking:** the :ref:`Tracker <fr.inria.built-in.Tracker>` node tracks multiple points, with the tracking engine of Blender (libmv).
* **Python scripting:** parameter expressions, user parameters, Python group plug-ins (PyPlugs), a script editor, callbacks, and GUI extensions with PySide.
* **Hardware:** a 64-bit x86-64 or arm64 processor and an OpenGL 2.0 graphics card. See :ref:`system-requirements`.
