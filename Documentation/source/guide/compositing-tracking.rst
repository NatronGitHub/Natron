.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _compositing-tracking:

Tracking and stabilizing
========================

The :ref:`Tracker <fr.inria.built-in.Tracker>` node follows one or more
points of an image over time. It uses the tracker of the libmv library, which
comes from Blender. The tracks can then be used to:

- **stabilize** the image, so that the tracked points stay still;
- **match-move** another image or a Roto shape, so that it follows the tracked
  points;
- remove or add **jitter** (camera shake).

This page first shows how to track points with the Tracker, then how the
tracks are turned into a Transform or a CornerPin.


Tracking points
---------------

Creating the Tracker
~~~~~~~~~~~~~~~~~~~~

1. Select the node whose image contains the features to track (usually a Read
   node), and create a Tracker node from the **Transform** group of the
   toolbar (see :doc:`getstarted-environment-toolbar`), or press :kbd:`Tab` in the Node Graph and type "Tracker".
2. Connect a Viewer to the Tracker and open its Properties panel. While the
   panel is open, the Viewer shows the Tracker tools in the Viewer toolbar
   above the image, and the tracks over the image.
3. In the **Transform** tab, keep **Motion Type** set to **None** while you
   track. In this mode the Tracker passes its input through unchanged, so the
   Viewer shows the image that the tracks follow.

Adding tracks
~~~~~~~~~~~~~

Look for small, contrasted features that stay visible during the shot:
corners, dots, the corner of a window. Avoid reflections, straight edges along
which the pattern could slide, and features that are hidden by another object
for part of the shot.

To add a track, do one of:

- hold :kbd:`Ctrl` + :kbd:`Alt` (:kbd:`Cmd` + :kbd:`Alt` on macOS) and click
  on the feature in the Viewer;
- turn on the **Add Track** button in the Viewer toolbar, then click in the
  Viewer once for each new track;
- click the **+** button below the track table in the Properties panel.

A track that is not selected is drawn as a cross on its center. A selected
track shows:

- the **center**: the point that is tracked;
- the **pattern** (the inner rectangle): the part of the image around the
  center that the tracker looks for in the other frames;
- the **search window** (the outer rectangle): how far from its previous
  position the tracker searches for the pattern in the next frame. Make it
  larger for fast motion; a larger search window is slower to track;
- the path of the center over the tracked frames.

Drag the center to move the track, and drag the corners and the middle of the
edges of the rectangles to resize the pattern and the search window. The
default sizes of new tracks are set by **Default Pattern Size** and **Default
Search Area Size** in the **Tracking** tab.

The pattern and the search window can be moved away from the center with an
**offset**. This is useful when the point you need cannot be tracked itself,
for example because it leaves the frame, is hidden, or has no texture: put the
center on the point you need, then drag the center with :kbd:`Ctrl`
(:kbd:`Cmd` on macOS) held to move the pattern onto a nearby feature that can
be tracked. Only the offset changes: the center, which is the point that the
track outputs, stays where it is, and follows the motion of the pattern. The
**Reset Offset** button removes the offset.

The track table in the Properties panel lists all tracks, with their
**Enabled** checkbox, label, **Motion-Model**, center, offset and error. You
can select tracks in the table or in the Viewer: click a track,
:kbd:`Shift` + click to add it to the selection, or drag a selection
rectangle. The buttons below the table add, remove, select all and reset
tracks, and **Average** creates a new track that is the average of the
selected tracks.

The tracks are drawn in the overlay color of the Tracker, which can be changed
with the overlay color button at the top of its Properties panel. Tracks that
are disabled at the current frame are drawn darker.

Choosing how a track is searched
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **Motion model** of each track (in the table, or **Default Motion
Model** for new tracks) tells the tracker how the pattern may change from one
frame to the next: translation only (the default), translation with rotation
and/or scale, affine, or perspective. Use the simplest model that matches the
motion of the feature. The other models are slower, and are only needed when
the pattern rotates, changes size or is distorted noticeably during the shot.

The other parameters of the **Tracking** tab apply to all tracks:

- **Track Red**, **Track Green**, **Track Blue**: the channels used to compare
  the pattern with the image.
- **Max. Error**: a track fails on a frame if its error (1 minus the
  normalized cross-correlation between the pattern and the image) is above this
  value.
- **Maximum Iterations**, **Use Brute-Force Pre-track**, **Normalize
  Intensities**, **Pre-blur Sigma**: settings of the libmv tracker. Normalize
  Intensities helps when the lighting changes during the shot.
- **Animate Enabled**: when checked, the **Enabled** parameter of a track is
  turned off at the frames where the track fails, so that the track is not
  used to compute the transform where it is wrong. You can then add new tracks
  to replace it.
- **Use Pattern Matching**: track with the TrackerPM plug-in (a simpler
  pattern-matching method, with its own **Score Type**) instead of libmv.

Running the tracker
~~~~~~~~~~~~~~~~~~~

Select the tracks to track, then use the tracking buttons of the Viewer
toolbar. Only the selected tracks that are enabled are tracked.

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Button
     - Default shortcut
     - What it does
   * - Track Backward
     - :kbd:`Z`
     - Track backward, up to the first frame of the timeline.
   * - Track Previous
     - :kbd:`X`
     - Track one frame backward.
   * - Stop Tracking
     - :kbd:`Esc`
     - Stop the current tracking.
   * - Track Next
     - :kbd:`C`
     - Track one frame forward.
   * - Track Forward
     - :kbd:`V`
     - Track forward, up to the last frame of the timeline.
   * - Track Range
     -
     - Ask for a first frame, a last frame and a step, then track over that
       range.
   * - Track All Keyframes
     - :kbd:`Ctrl` + :kbd:`V` (:kbd:`Cmd` + :kbd:`V` on macOS)
     - Track across all the pattern keyframes of the selected tracks.
   * - Track Current Keyframe
     - :kbd:`Ctrl` + :kbd:`C` (:kbd:`Cmd` + :kbd:`C` on macOS)
     - Track only over the pattern keyframes around the current frame.

These shortcuts work when the Viewer has the keyboard focus (click in it
first), and can be changed in the Shortcut Editor
(**Edit > Preferences > User Interface > Shortcut Editor**).

The other buttons of the Viewer toolbar are:

- **Clear Animation**, **Clear Animation Backward**, **Clear Animation
  Forward**: delete the tracked positions of the selected tracks, at all frames,
  or before or after the current frame.
- **Refresh Viewer**: redraw the image at each tracked frame, not only the
  tracks. Tracking is faster when this is off.
- **Center Viewer**: keep the selected tracks at the center of the Viewer
  during tracking.
- **Create Keyframe On Move**: when on (the default), moving a track in the
  Viewer sets a pattern keyframe (see below).
- **Show Error**: color the path of each track by its error at each frame,
  from green (good match) to red (bad match).
- **Set Keyframe On Pattern**, **Remove Keyframe On Pattern**: add or remove a
  pattern keyframe on the selected tracks at the current frame.
- **Reset Offset**, **Reset Track**: remove the offset, or reset the pattern,
  the search window and the animation of the selected tracks.
- **Mag. Window Size**: the size of the magnified view of the selected track.

While a track is selected, the Viewer also shows a magnified view of its
pattern, and thumbnails of its pattern keyframes. Click a thumbnail to go to
that frame.

Fixing a track with pattern keyframes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The tracker looks for the pattern as it appears at a **pattern keyframe**.
When a track is added in the Viewer while **Create Keyframe On Move** is on,
its first pattern keyframe is set at that frame. Otherwise, the first one is
set at the frame where the track is first tracked. When a feature changes over
the shot (it turns, gets bigger, the light changes), a single reference is no
longer enough, and the track drifts or fails.

To fix it:

1. Go to the frame where the track starts drifting.
2. Move the track back onto the feature in the Viewer. With **Create Keyframe
   On Move** on, this sets a pattern keyframe at this frame. You can also use
   **Set Keyframe On Pattern**.
3. Track again from this frame, or use **Track All Keyframes** to track
   between the keyframes.

Between two pattern keyframes, the tracker uses the pattern of the nearest
one. The pattern keyframes of the selected tracks are shown on the timeline.
Above the track table, "Track keyframe: N of M" shows them, with buttons to go
to the previous or next pattern keyframe, and to add, remove or clear them.


Using the tracks
----------------

Stabilizing and match-moving
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **Transform** tab turns the tracks into a transformation:

- **Motion Type**:

  - **None**: the Tracker is a pass-through (use it while tracking);
  - **Stabilize**: transform the input image so that the tracked points stay
    still;
  - **Match-Move**: transform an image so that it follows the tracked points;
  - **Remove Jitter**: keep the overall motion but remove its high
    frequencies (the shake), by averaging over **Jitter Period** frames;
  - **Add Jitter**: apply only the high frequencies of the motion, to add a
    camera shake that follows the original one.

- **Transform Type**: **Transform** (translation, rotation and scale) or
  **CornerPin** (the default, a perspective transform, see
  :ref:`tracking-model`).
- **Reference Frame**: the frame at which the transform is the identity, for
  example the frame where the element to match-move is in the right place.
  **Set to Current Frame** sets it to the current frame.

The transform is computed again at the end of every tracking run. Press
**Compute** to compute it after other changes, or check **Compute Transform
Automatically** to compute it whenever a parameter that affects it changes.

The Tracker can apply the transform to its own input, which is how you
stabilize the tracked image. You can also export the transform to another
node: in the **Export** section of the Transform tab, press **Export** to
create a :ref:`CornerPin <net.sf.openfx.CornerPinPlugin>` or a
:ref:`Transform <net.sf.openfx.TransformPlugin>` node (depending on
**Transform Type**) next to the Tracker. **Motion Type** must not be None.
The new node is not connected: insert it in the branch that must move, for
example after the Roto node or the image to match-move, before it is merged
onto the plate.

- **Link**: when checked, the parameters of the new node are linked to the
  Tracker, so they follow any change made later in the Tracker. When unchecked,
  they are a copy. Keep it checked for a match-move, so that the exported node
  stays up to date when you fix a track or change the reference frame.
- **Use Current Frame**: when checked, the exported node is the identity at the
  current frame instead of the reference frame.

Match-moving a Roto shape or a texture on a flat surface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To make a Roto mask or an image follow a flat surface (a wall, a screen, a
sign):

1. Track several points on the surface, all in the same plane.
2. In the Transform tab, set **Motion Type** to **Match-Move** and
   **Transform Type** to **CornerPin**, and set the **Reference Frame** to the
   frame where the shape is drawn or the image is placed.
3. Check **Disable Transform**, so that the Viewer shows the input unchanged
   while you edit the CornerPin points.
4. At the reference frame, set the **From** points of the CornerPin controls,
   for example to the bounding box of the Roto shape. **Set to Input Rod**
   sets them to the rectangle of the input image.
5. Press **Compute**, since editing the From points by hand does not
   recompute the transform, then **Export** with **Link** checked.
6. Insert the exported CornerPin after the Roto node or the texture.

See :doc:`compositing-rotopaint` for drawing the shapes, and
:doc:`compositing-transform` for the transform nodes. To link parameters by
hand, drag a parameter onto another one while holding :kbd:`Ctrl`
(:kbd:`Cmd` on macOS).


.. _tracking-model:

How the transform is computed
-----------------------------

A Transform computed from tracks is a
`similarity <https://en.wikipedia.org/wiki/Similarity_(geometry)>`_:
translation, rotation and uniform scale. It cannot represent a change of
perspective. A CornerPin can: it is a
`homography <https://en.wikipedia.org/wiki/Homography>`_, which covers all
the perspective transforms of a plane, without lens distortion.

The number of tracks enabled at a frame limits what can be found at that
frame. With one track, a Transform or a CornerPin is only a translation. With
two tracks, both are a similarity. With three tracks, a CornerPin is an
affine transform (it can also shear and scale differently along two
directions). A CornerPin needs at least four tracks to include a change of
perspective.

The Tracker does not map one track to each corner. It looks for the model
(the similarity or the homography) that best fits all the tracks. At each
frame, each track gives a correspondence: its position at the reference frame
and its position at that frame. The model kept is the one closest to all
these correspondences. The more tracks you use, the more robust the result
is in the area covered by the tracks.

The correspondences used at a frame come from the tracks that are enabled at
that frame (the **Enabled** parameter, which can be animated) and that have a
tracked position at that frame.

**Fitting Error (px)** shows, for each frame, how far in pixels the tracked
points are from the reference points moved by the computed model (a root mean
square over all tracks). It estimates the quality of the model. A warning
("Incorrect model") is shown when it reaches **Warn If Error Is Above**.

The transform is computed as a second step, after tracking, over all the
tracked frames at once (at the end of each tracking run, when **Compute** is
pressed, or when a parameter changes with **Compute Transform Automatically**
checked), because the result at one frame depends on the other
frames when smoothing or jitter is used. The parameters that change the result
are:

- **Motion Type**;
- **Transform Type** (Transform or CornerPin);
- **Reference Frame**;
- **Jitter Period**;
- **Smooth**: average the result over this number of frames, to remove high
  frequency noise. In Add Jitter mode, you can raise the high frequencies to
  simulate a camera shake that follows the original camera motion;
- **Robust Model**: when checked, tracks that do not fit the model (wrong
  tracks, called *outliers*) are ignored when computing it. Keep it checked in
  most cases. If the tracked points cannot be described by the chosen model
  (for example points that are not on one plane, for a CornerPin), the
  computation may fail; unchecking it uses all the tracks, and gives a model
  that averages their motion;
- the **Enabled** parameter of each track.

For a CornerPin, the **To** points are computed from the **From** points: at
each frame, the transformation found from the tracks is applied to the From
points to get the To points. If you change the From points (for example with
**Set to Input Rod**), the transform must be computed again for all frames.
