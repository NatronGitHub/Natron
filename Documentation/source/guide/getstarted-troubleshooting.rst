.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Troubleshooting
===============

Natron has bugs, as any software does.

Natron is also a free and open-source software, and bugs are fixed by volunteers when they have some spare time, so please be tolerant and do not expect your bug to be fixed within the hour. It may take days, weeks, or it may even never be fixed.

Properly reporting a bug takes time, but the time spent reporting a bug will certainly help you and the community a lot. It is also the best way to find a temporary solution or a workaround.


Identifying the Kind of Bug
---------------------------

Natron may fail in several ways:

#. It crashes while doing some specific user interaction with the GUI.
#. It crashes while rendering the project.
#. Rendered images are wrong, or contain black areas.
#. Natron hangs and the GUI is not responsive (i.e. menus and buttons do not respond). This is probably a *deadlock* in the GUI code of Natron.
#. Rendering stops before the end of the sequence. This is probably caused by an OpenFX error: check the error log from the "Display/Show Project Errors Log..." menu: there may be an indication of the problem (but it can still be a Natron bug).
#. Rendering hangs or Natron hangs, but the GUI is responsive. This is probably a *deadlock* in the rendering code, and this is the hardest kind of bug to reproduce or fix. If it cannot be reproduced easily, then your best bet is to use one of the workarounds below.


Searching and Reporting Bugs
----------------------------

Bugs may come from OpenFX plugins that were not bundled with Natron, so before reporting anything, if you have any extra OpenFX plugins installed, uncheck "Enable default OpenFX plugins location" in "Preferences/Plug-ins", save preferences, relaunch Natron, and check that the bug is still here.

The best way to have your bug considered for fixing is to first search on the `Natron forum <https://discuss.pixls.us/c/software/natron>`_ and in the `Natron issues <https://github.com/NatronGitHub/Natron/issues>`_ if this is a known bug. If yes, then read about it, and try some workarounds given in these bug reports (see below for more workarounds).

If this bug does not seem to be a known issue, then post a `new issue <https://github.com/NatronGitHub/Natron/issues/new>`_ on the Natron github, and follow strictly the guidelines to report the bug. The issue title should be as precise as possible ("Natron crash" is *not* a correct title, see existing issues for title examples). If possible, also post a project that exhibits the issue. Make the project as small as possible: remove extra assets or replace them by small JPEG sequences, checkerboards or colorwheels, etc. You can then either attach your project as a zip file to the github issue, or post a link to a file sharing service.


Common Workarounds
------------------

Luckily, there are workarounds for most Natron crashes or hangs. Here are a few one worth trying, but of course your mileage may vary or you may find another workaround (which you should describe in the proper `Natron issue <https://github.com/NatronGitHub/Natron/issues>`_).

#. Avoid using videos with inter-frame compression as inputs and outputs. This includes H.264 (eg AVCHD) and H.265 (HEVC) video. ProRes is OK but slow, especially for writing. DNxHR is OK. Individual frames are *best* (DPX, EXR, TIFF, PNG, JPEG, whatever suits your input video quality and bit depth). The video reader is here for convenience, but it may have difficulties decoding some videos. The video writer may also be a source of bugs, and should be avoided for long sequences: if Natron crashes in the middle, then the whole sequence has to be rendered. Extract individual frames, do your compositing, then compress the frames (and optionally mux the audio) with an external tool. To extract frames, you may use a simple Natron project or any other tool (e.g. `FFmpeg <https://www.ffmpeg.org/ffmpeg.html>`_). To compress frames to a video, there are also many tools available, e.g. `FFmpeg <https://www.ffmpeg.org/ffmpeg.html>`_, `MEncoder <https://en.wikipedia.org/wiki/MEncoder>`_, or `VirtualDub <http://virtualdub.sourceforge.net/>`_ (windows-only). This is the standard compositing workflow and the preferred method of running Natron. See the :doc:`tutorial on how to convert videos to image sequences <tutorials-imagesequence>`.
#. If Natron hangs or crashes when rendering an image sequence (this does not work when rendering to a video), check that the rendered frames are OK, relaunch Natron and in the parameters of the Write node uncheck "Overwrite". That way, only the missing frames will be rendered.
#. If you have a large project, or a project with heavy processing, use the :ref:`DiskCache Plugin <fr.inria.built-in.DiskCache>` at places that make sense: downstream heavy processing in the graph, or before you use the result of processing as inputs to :ref:`Roto <fr.inria.built-in.Roto>` or :ref:`RotoPaint <fr.inria.built-in.RotoPaint>`.
#. On multicore computers (e.g. Threadripper), go to Edit => Preferences => Threading and under Number of parallel renders limit it to "8".

You will quickly notice that using individual frames instead of videos for inputs and output give a *big* performance boost and will most probably solve your issues, so once you've learned how to decompress/compress any video, this will become your standard workflow. Just add extra disk space, and you're good to do serious and fluid compositing with Natron.


OpenGL/GPU Rendering Issues
---------------------------

If the viewer displays some error message about OpenGL, or if the Viewer display is corrupted, then GPU rendering is probably going bad. Note that this kind of problem seems to only happen on Windows, so you might want to consider switching to Linux or macOS to use Natron if your GPU is not well supported by Natron under Windows.

#. Update you graphics driver to at least the latest stable version. On Windows, make a restoration point before updating the driver, and case anything goes wrong.

#. For systems with integrated and discrete GPUs (e.g. Intel and NVIDIA), configure the system so that Natron uses the discrete GPU. For NVIDIA discrete GPUs, this is done by opening the NVIDIA control panel, select "Manage 3D Settings", then "Program Settings", and for Natron select the NVIDIA GPU as the preferred graphics processor.

#. Create a :ref:`Shadertoy <net.sf.openfx.Shadertoy>`, click "Renderer Info..." and check that the OpenGL version is at least 2.1 and that the extension ``GL_ARB_texture_non_power_of_two`` is available. If the displayed info does not correspond to your graphics card, check that the OpenGL drivers for your card are installed. If not, install the software called "OpenGL Extension Viewer" and check that your card appears in the list of renderers. If not, then it is a drivers issue.
#. In Natron Preferences / GPU Rendering, check that the displayed is consistent with what "Renderer Info..." above gave.
#. Now uncheck "Enable GPU Render" in the Shadertoy node and click the refresh/recycle button on the top of the viewer. Click again "Renderer Info..." and it should say it now uses Mesa in the ``GL_VERSION``. Does it fix the issue? If yes, you may try the next step to globally disable OpenGL rendering in Natron.
#. To temporarily fix this issue, in Natron Preferences / GPU Rendering, set "OpenGL Rendering" to "Disabled", click the "Save" button in the Preferences window, quit Natron, launch Natron, check that GPU rendering is still disabled in the Preferences, and test your project.


| If you there is an error similar to ``Shadertoy3: Can not render: glGetString(GL_VERSION) failed.`` 
| Go to File => Preferences => GPU Rendering and set No. of OpenGL Context to 5
| Save and relaunch Natron.
