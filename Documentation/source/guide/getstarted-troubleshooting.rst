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
#. Rendering stops before the end of the sequence. This is probably caused by an OpenFX error: check the log in Display → Show Project Errors Log...: there may be an indication of the problem (but it can still be a Natron bug).
#. Rendering hangs or Natron hangs, but the GUI is responsive. This is probably a *deadlock* in the rendering code, and this is the hardest kind of bug to reproduce or fix. If it cannot be reproduced easily, then your best bet is to use one of the workarounds below.


Searching and Reporting Bugs
----------------------------

Bugs may come from OpenFX plugins that were not bundled with Natron, so before reporting anything, if you have any extra OpenFX plugins installed, uncheck "Enable default OpenFX plugins location" in Edit → Preferences → Plug-ins, click "Save", restart Natron, and check that the bug is still there.

The best way to have your bug considered for fixing is to first search on the `Natron forum <https://discuss.pixls.us/c/software/natron>`_ and in the `Natron issues <https://github.com/NatronGitHub/Natron/issues>`_ if this is a known bug. If yes, then read about it, and try some workarounds given in these bug reports (see below for more workarounds).

If this bug does not seem to be a known issue, open a `new issue <https://github.com/NatronGitHub/Natron/issues/new>`_ on GitHub, and follow the guidelines to report the bug. The issue title should be as precise as possible ("Natron crash" is *not* a correct title, see existing issues for title examples). If possible, also post a project that exhibits the issue. Make the project as small as possible: remove extra assets or replace them by small JPEG sequences, checkerboards or colorwheels, etc. You can then either attach your project as a zip file to the github issue, or post a link to a file sharing service.


Known Bugs and Workarounds
--------------------------   

This list is incomplete. The numbers (#…) are the GitHub issue numbers.

There are workarounds for most Natron crashes and hangs. Here are a few worth trying, but of course your mileage may vary or you may find another workaround (which you should describe in the proper `Natron issue <https://github.com/NatronGitHub/Natron/issues>`_).

- Avoid using videos with inter-frame compression as inputs and outputs. This includes H.264 (eg AVCHD) and H.265 (HEVC) video. ProRes is OK but slow, especially for writing. DNxHR is OK. Individual frames are *best* (DPX, EXR, TIFF, PNG, JPEG, whatever suits your input video quality and bit depth). The video reader is here for convenience, but it may have difficulties decoding some videos. The video writer may also be a source of bugs, and should be avoided for long sequences: if Natron crashes in the middle, then the whole sequence has to be rendered. Extract individual frames, do your compositing, then compress the frames (and optionally mux the audio) with an external tool. To extract frames, you may use a simple Natron project or any other tool (e.g. `FFmpeg <https://www.ffmpeg.org/ffmpeg.html>`_). To compress frames to a video, use FFmpeg or any other video encoder. This is the standard compositing workflow and the preferred method of running Natron. See the :doc:`tutorial on how to convert videos to image sequences <tutorials-imagesequence>`.
- If Natron hangs or crashes when rendering an image sequence (this does not work when rendering to a video), check that the rendered frames are OK, relaunch Natron and in the parameters of the Write node uncheck "Overwrite". That way, only the missing frames will be rendered.
- If you have a large project, or a project with heavy processing, use the :ref:`DiskCache Plugin <fr.inria.built-in.DiskCache>` at places that make sense: downstream heavy processing in the graph, or before you use the result of processing as inputs to :ref:`Roto <fr.inria.built-in.Roto>` or :ref:`RotoPaint <fr.inria.built-in.RotoPaint>`.
- On computers with many cores (e.g. AMD Threadripper), set "Number of parallel renders" to 8 in Edit → Preferences → Threading.

Using individual frames instead of videos for input and output gives a *big* performance boost and will most probably solve your issues, so once you've learned how to decompress/compress any video, this will become your standard workflow. It only needs more disk space.

User Interface bugs
...................

- Renaming a node often fails `\#664 <https://github.com/NatronGitHub/Natron/issues/664>`_: if the Properties panel does not let you rename a node, select it in the node graph and press N. If that fails too, rename it from the Script Editor, e.g. ``app1.Blur1.setLabel("MyBlur")``.

Roto Node bugs
..............

- The Roto and RotoPaint mask input does not work `\#367 <https://github.com/NatronGitHub/Natron/issues/367>`_: This feature is not yet implemented.
- RotoPaint clone tool: the source type does the same thing when set to "background" and to "foreground" `\#629 <https://github.com/NatronGitHub/Natron/issues/629>`_: Use multiple rotopaint nodes.



OpenGL/GPU Rendering Issues
---------------------------

If the viewer displays an error message about OpenGL, GPU rendering probably fails. Most such reports come from Windows, often with outdated graphics drivers: first update the driver of your graphics card.

#. Create a :ref:`Shadertoy <net.sf.openfx.Shadertoy>`, click "Renderer Info..." and check that the OpenGL version is at least 2.1 and that the extension ``GL_ARB_texture_non_power_of_two`` is available. If the displayed info does not correspond to your graphics card, check that the OpenGL drivers for your card are installed. If not, install the software called "OpenGL Extension Viewer" and check that your card appears in the list of renderers. If not, then it is a drivers issue.
#. In Edit → Preferences → GPU Rendering, check that "Active OpenGL renderer" matches what "Renderer Info..." showed.
#. Now uncheck "Enable GPU Render" in the Shadertoy node and click the refresh/recycle button on the top of the viewer. Click again "Renderer Info..." and it should say it now uses Mesa in the ``GL_VERSION``. Does it fix the issue? If yes, you may try the next step to globally disable OpenGL rendering in Natron.
#. To work around the problem, set "OpenGL Rendering" to "Disabled" in Edit → Preferences → GPU Rendering, click "Save", restart Natron, check that GPU rendering is still disabled in the preferences, and test your project.


If there is an error similar to ``Shadertoy3: Can not render: glGetString(GL_VERSION) failed.``, set "No. of OpenGL Contexts" to 5 in Edit → Preferences → GPU Rendering, click "Save" and restart Natron.
