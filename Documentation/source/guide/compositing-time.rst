.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Editing time
============

Natron is a compositor, not an editing application, but it can shift, trim,
freeze, reverse, retime and join clips. This page describes where each of
these operations is done. Most nodes mentioned here are in the **Time** menu
of the toolbar.

Each node in the graph has a frame range. A Read node has the range of its
file; the nodes below it usually have the range of their input. The project
frame range, set in the Project Settings, is the range shown in the timeline
and rendered by default (see :doc:`compositing-projects-framerange`).

In the Read node
----------------

The Read node already has the most common timing controls:

- **First Frame** and **Last Frame** trim the part of the file that is used;
- **Before** and **After** set what the node outputs outside that range:
  **Hold** the first or last frame (the default), **Loop**, **Bounce**
  (play back and forth), **Black**, or an **Error**;
- **Frame Mode** chooses whether the clip is placed with a **Starting
  Time** (the frame of the timeline where the clip starts) or with a **Time
  Offset** (a number of frames added to the frame numbers of the file).

Use these first, before adding time nodes.

Shifting, trimming and freezing
-------------------------------

:ref:`TimeOffset <net.sf.openfx.timeOffset>`
   Moves its input in time: with a **Time Offset (Frames)** of 10, frame 1
   of the input appears at frame 11. **Reverse Input** plays the clip
   backwards.

:ref:`FrameRange <net.sf.openfx.FrameRange>`
   Sets the frame range of its input anywhere in the graph, and what to
   output before and after it (**Original**, **Hold**, **Black**, **Loop**
   or **Bounce**). It is similar to Nuke's TimeClip, and is often used
   before AppendClip.

:ref:`FrameHold <net.sf.openfx.FrameHold>`
   With an **Increment** of 0, outputs the frame given by **First Frame** at
   every frame. With an increment of 2, it holds every other frame for two
   frames ("on twos"), and so on. FrameHold is also useful while working:
   freezing a heavy branch of the graph on one frame makes the rest of the
   graph faster to check, as long as you remove it afterwards.

In the :doc:`Dope Sheet <getstarted-environment-anim>`, the Read, TimeOffset
and FrameRange nodes appear as bars that can be dragged to move them in time,
and the ends of a Read node's bar can be dragged to trim it.

Changing the speed
------------------

The :ref:`Retime <net.sf.openfx.Retime>` node changes the speed of a clip.

- **Speed** is the playback speed: 0.5 plays the clip at half speed, 2 at
  double speed, and a negative speed plays it backwards. Speed can be
  animated, to accelerate or slow down a clip progressively. Note that
  Natron integrates the speed over time to find the input frame, so an
  animated speed gives a smooth change; this is not the same as the speed of
  Nuke's Retime node.
- **Warp** is a curve that remaps the input frames to the output frames,
  for more control: a flat part freezes the clip, a negative slope plays it
  backwards.
- **Filter** sets how frames that fall between two input frames are
  computed: **Nearest** picks the closest frame, **Linear** blends the two
  nearest ones (the default), and **None** asks the input node for the
  fractional frame, which is useful only if that node can render it itself,
  for example an animated Transform or Roto.

A frame computed by blending two frames shows a double image on moving
objects. The nodes distributed with Natron do not compute new frames from
the motion in the image (optical flow retiming, such as the Oflow node of
Nuke). For strong slow motion of live footage, render the slowed-down clip
with an external tool that does motion interpolation, and read the result in
Natron.

Joining and dissolving clips
----------------------------

:ref:`AppendClip <net.sf.openfx.AppendClip>`
   Plays its inputs (**1**, **2**, **3**, ...) one after the other, starting
   at **First Frame**. **Cross Dissolve** overlaps consecutive clips by that
   number of frames and dissolves between them. **Fade In** and **Fade
   Out** fade the start of the first clip from black and the end of the last
   one to black. Use FrameRange nodes before it to trim the clips.

:ref:`TimeDissolve <net.sf.openfx.TimeDissolvePlugin>` (**Merge** menu)
   Dissolves from input **A** to input **B**: before the frame **In** it
   outputs A, after the frame **Out** it outputs B. The **Curve** parameter sets the shape of the transition, for
   example to make it start slowly. Unlike AppendClip, it does not move the
   clips in time.

Blending frames together
------------------------

:ref:`FrameBlend <net.sf.openfx.FrameBlend>`
   Combines several frames of its input, for example the average of the
   current frame and the five previous ones (**Frame Range** and
   **Operation**). Averaging
   frames removes noise from a static shot. With the **Input Range** button
   and a matte of the moving objects in the **FgM** input, it can build a
   clean plate of the background, in which only the pixels that are not
   covered by the foreground are averaged.

:ref:`TimeBlur <net.sf.openfx.TimeBlur>`
   Averages the input over a fraction of a frame, given by **Shutter** and
   **Divisions**. This adds motion blur to animated elements (Transforms,
   Roto shapes, ...) that do not have their own motion blur settings, at the
   cost of rendering the input several times per frame.
   :ref:`NoTimeBlur <net.sf.openfx.NoTimeBlurPlugin>`, placed above it,
   rounds the time back to whole frames for the part of the graph that should
   not be blurred.

Further reading
---------------

The archived pages of the Optical Enquiry Nuke wiki on `the Time nodes
<https://web.archive.org/web/20220524010817/http://www.opticalenquiry.com/nuke/index.php?title=The_Time_Nodes>`__
describe the equivalent Nuke nodes (TimeOffset, FrameHold, Retime,
AppendClip, TimeClip, Oflow).
