.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Features
========

.. toctree::
   :maxdepth: 2
   
* 32 bits floating point linear color processing pipeline.
* Colorspace management handled by the famous open-source OpenColorIO library.
* Dozens of file formats supported: EXR, DPX,TIFF, PSD, SVG, Raw, JPG, PNG …
* **Support for many free and open-source OpenFX plugins:**
  * `OpenFX-IO <https://github.com/NatronGitHub/openfx-io>`_
  * `OpenFX-Misc <https://github.com/NatronGitHub/openfx-misc>`_
  * `OpenFX-Arena <https://github.com/NatronGitHub/openfx-arena>`_
  * `OpenFX-G'MIC <https://github.com/NatronGitHub/openfx-gmic>`_
  * `OpenFX-OpenCV <https://github.com/NatronGitHub/openfx-opencv>`_
  * `OpenFX-Yadif <https://github.com/NatronGitHub/openfx-yadif>`_ deinterlacer
  * `OpenFX-Vegas <https://github.com/NatronGitHub/openfx-vegas>`_  SDK samples
  * `OpenFX <https://github.com/NatronGitHub/openfx>`_ samples
  * `TuttleOFX <https://github.com/tuttleofx/TuttleOFX>`_
* **Support for commercial OpenFX plugins:**
  * `RevisionFX <http://www.revisionfx.com/>`_ products
  * `NeatVideo <https://www.neatvideo.com/>`_ denoiser
  * `Furnace <http://www.thefoundry.co.uk/products/furnace/>`_ by The Foundry
  * `KeyLight <http://www.thefoundry.co.uk/products/plugins/keylight/>`_ by The Foundry
  * GenArts `Sapphire <http://www.genarts.com/software/sapphire/overview>`_
  * Other `GenArts <http://www.genarts.com>`_ products
  * And many more....
  * OpenFX v1.4 supported
* **Intuitive user interface:** Natron aims not to break habits by providing an intuitive and familiar user interface.  It is possible to separate on any number of screens the graphical user interface. It supports Retina screens on MacOSX.
* **Performances:**  Never wait for anything to be rendered, in Natron anything you do produces real-time feedback thanks to its optimised multi-threaded rendering pipeline and its support for proxy rendering (i.e: the render pipeline can be computed at lower res to speed-up rendering).
* **Multi-task:** Natron can render multiple graphs at the same time and make use of 100% of the compute power of your CPU.
* **Network rendering:** Natron can be used as a command-line tool and can be integrated on a render farm manager such as `Afanasy <http://cgru.info/home>`_.
* **NatronRenderer:** A command line tool for execution of project files and python scripts. The command line version is executable from ssh on a computer without any display.
* **Fast & interactive Viewer** – Smooth & accurate  zooming/panning even for very large image sizes (tested on 27k x 30k images).
* **Real-time playback:** Natron offers  a real-time playback with thanks to its RAM/Disk cache technology. Once a frame is rendered, it can be reproduced instantly afterwards, even for large image sizes.
* **Low hardware requirements:** All you need is an x86 64 bits or 32 bits processor, at least 3 GB of RAM and a graphic card that supports OpenGL 2.0 or OpenGL 1.5 with some extensions.
* **Motion editing:** Natron offers a simple and efficient way to deal with keyframes with a very accurate and intuitive curve editor. You can set expressions on animation curves to create easy and believable motion for objects. Natron also incorporates a fully featured dope-sheet to quickly edit clips and keyframes in time-space.
* **Multi-view workflow:** Natron saves time by keeping all the views in the same stream. You can separate the views at any time with the OneView node.
* **Rotoscoping/Rotopainting:** Edit your masks and animate them to work with complex shots
* **Tracker node:** A point tracker is  embedded in Natron to track multiple points. Version 2.1 of Natron will incorporate the Tracker from Blender.
