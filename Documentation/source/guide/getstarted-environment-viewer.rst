.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Compositing viewers
===================

A Viewer panel displays the image produced by a **Viewer node** in the node
graph. Each Viewer node has its own panel. A project usually has one Viewer,
but you can add more with :menuselection:`Display --> New Viewer`
(:kbd:`Ctrl+I`), for example to look at two parts of the graph at the same
time.

The Viewer is not part of the final render: it only shows what the nodes
upstream of it compute. To write images to disk, use a
:ref:`Write node <fr.inria.built-in.Write>` (see
:doc:`compositing-projects-render`).

.. note::

   The shortcuts on this page are the default ones. They can be changed in
   the *Shortcut Editor* page of :menuselection:`Edit --> Preferences`,
   which also lists the ones not shown here. On macOS, :kbd:`Ctrl` means
   the :kbd:`Command` key and :kbd:`Alt` means the :kbd:`Option` key.

The panel has, from top to bottom:

- two rows of controls (the *top toolbar*): what to display and how;
- the image area, where the nodes also draw their interactive handles
  (*overlays*: transform handles, roto shapes, trackers, ...);
- one or two *information bars*;
- the *player* controls and the *timeline*.


Choosing what to view: inputs A and B
-------------------------------------

A Viewer node has up to 10 inputs. Connect nodes to them in the node graph,
by dragging the input arrows of the Viewer node, or with the number keys:

- select a node in the node graph and press :kbd:`1` to :kbd:`0` to connect
  it to Viewer input 1 to 10 and show it on the **A** side;
- press :kbd:`Shift+1` to :kbd:`Shift+0` to connect it and show it on the
  **B** side;
- with no node selected (or with the Viewer node selected), the same keys
  only switch the A or B side to that Viewer input.

If your keyboard layout does not have digits on the top row of keys, or with
some remote display setups, see *Use number keys for the viewer* in the
:doc:`Preferences </_prefs>`.

In the top toolbar, the **A** and **B** menus select which of the connected
inputs is shown on each side. :kbd:`Return` swaps A and B.


Comparing A and B: wipe and stack modes
---------------------------------------

The menu between **A** and **B** sets how the two sides are combined. In
the formulas, *a* is the alpha of A and *d* is the wipe mix factor:

===============  ==========================================================
``-``            Only A is shown (default).
``W-Under``      Wipe: A on one side of the wipe line, B on the other side,
                 mixed with *d*: A(1 - d) + Bd.
``W-Over``       Wipe, with B over A: A + B(1 - a)d.
``W-Minus``      Wipe, A minus B.
``W-OnionSkin``  Wipe, A plus B.
``S-Under``      Stack: B only, over the whole image.
``S-Over``       Stack: B over A, A + B(1 - a).
``S-Minus``      Stack: A minus B.
``S-OnionSkin``  Stack: A plus B.
===============  ==========================================================

:kbd:`W` turns the wipe (``W-Under``) on and off.

In the wipe modes, a handle is drawn on the image:

- drag its center to move the wipe line, or press :kbd:`Shift+F` to center
  it on the mouse pointer;
- drag the line to rotate it;
- drag the arc handle to change the mix factor *d*.

When A and B are combined, a second information bar shows the values of the
B side. The *Automatically enable wipe* preference turns the wipe on when you
change a Viewer input with the mouse over the Viewer.


Layers, channels and alpha
--------------------------

The first three menus of the top toolbar select which data are displayed.

**Layer**
   The layer (plane) that the Viewer fetches from its input: ``Color.RGBA``,
   or any other layer produced upstream, such as motion vectors. Its channels
   are mapped to the red, green, blue and alpha of the Viewer in order (a
   two-channel ``UV`` layer is shown as red and green).
   :kbd:`Page Up` and :kbd:`Page Down` switch to the previous and next layer.

**Alpha channel**
   The channel, from any layer, that is used as alpha when displaying
   *Alpha* or *Matte* below.

**Display channels**
   ============  ========  ===================================================
   Luminance     :kbd:`Y`  A grayscale image of the luminance.
   RGB                     The color channels (default).
   Red           :kbd:`R`  Only red, as a grayscale image.
   Green         :kbd:`G`  Only green.
   Blue          :kbd:`B`  Only blue.
   Alpha         :kbd:`A`  The channel chosen in *Alpha channel*.
   Matte         :kbd:`M`  The RGB image, with the alpha added as a red tint.
   ============  ========  ===================================================

   Pressing the same key again goes back to RGB. With :kbd:`Shift`
   (:kbd:`Shift+R`, ...), the setting applies to input A only.

The **checkerboard** button (second row) draws a checkerboard under A
instead of black, to show transparent areas. Its tile size and colors are set
in the *Viewer* section of the :doc:`Preferences </_prefs>`.


Navigating the image
--------------------

==============================  ==============================================
Pan                             Middle-button drag, or :kbd:`Alt` +
                                left-button drag
Zoom                            Mouse wheel (centered on the pointer), or
                                :kbd:`Alt` + middle-button drag
Zoom in / out                   :kbd:`+` / :kbd:`-`
Zoom to 100%                    :kbd:`Ctrl+1`
Fit the image in the Viewer     :kbd:`F`
Previous / next frame           :kbd:`Ctrl` + mouse wheel
Context menu                    Right-click
==============================  ==============================================

The **zoom** menu in the top toolbar shows the current zoom and offers preset
levels. Next to it, the **lock** button synchronizes all the Viewers, so
that they show the same part of the image, and the **fit** button does the
same as :kbd:`F`.

When you zoom out, the Viewer renders the image at a lower resolution, which
is faster.


Proxy mode
----------

Proxy mode renders everything upstream of the Viewer at a lower resolution,
to make interactive work and playback faster. The displayed image is less
detailed, but renders to disk are not affected.

- :kbd:`Ctrl+P` (or the proxy button) turns proxy mode on and off;
- the menu next to it sets the scale-down factor: 2, 4, 8, 16 or 32
  (:kbd:`Alt+1` to :kbd:`Alt+5`).

The *Automatically enable proxy when scrubbing the timeline* preference
turns proxy mode on while you drag in the timeline.


Region of interest and other render options
-------------------------------------------

Other buttons of the first row change which part of the image is computed,
and when:

**Clip to format** (:kbd:`Shift+C`, on by default)
   Only displays the part of the image that is inside its format. When off,
   the pixels outside the format are displayed too, since the region of
   definition can be larger than the format (for example after a Transform
   or a Blur).

**Full frame processing**
   By default the Viewer only renders the part of the image that is visible.
   When this button is on, it renders the whole image.

**User region of interest** (:kbd:`Shift+W`)
   Limits the part of the image that the Viewer keeps up to date to a
   rectangle, which is useful to work on a detail of a heavy composition.
   Drag its edges, corners or center to change it. :kbd:`Alt+W` turns it on
   and lets you drag a new rectangle.

**Refresh** (:kbd:`U`)
   Renders the current frame again. :kbd:`Ctrl+Shift+U` also opens the
   render statistics window, which shows where the render time was spent.

**Pause updates** (:kbd:`P` for input A only, :kbd:`Shift+P` for both inputs)
   The Viewer stops updating when you change parameters, so that you can
   change several of them before looking at the result.


Gain, gamma and display transform
---------------------------------

The second row of the toolbar changes how the image is shown on screen.
These settings only affect the display: they do not change the pixels seen
by other nodes, nor the renders.

**Gain**
   In f-stops: the image is multiplied by 2\ :sup:`gain` before display. The
   slider goes from -6 to +6. The button on its left switches between the
   neutral value (0) and your setting.

**Auto-contrast**
   Adjusts gain and offset automatically to the range of values in the
   visible part of the image. This is useful for images with values far
   outside the 0–1 range, such as depth or motion vectors.

**Gamma**
   Applied after the gain. The button on its left switches between 1.0 and
   your setting.

**Viewer color process**
   Natron works on linear values. This menu sets the transform from linear
   values to the values sent to your monitor: ``sRGB`` (default),
   ``Rec.709``, ``BT1886``, or ``Linear(None)`` (no transform).

The Viewer does not apply OpenColorIO display and view transforms itself. To
look at the image through an OpenColorIO display and view (for example a
film look), insert an :ref:`OCIODisplay node <fr.inria.openfx.OCIODisplay>`
just before the Viewer, and set the Viewer color process to
``Linear(None)``, since OCIODisplay already outputs display values. Do not
connect your Write nodes after that OCIODisplay node. The OpenColorIO
configuration used by all the OCIO nodes is set in the *Color Management*
section of the :doc:`Preferences </_prefs>`.

If the project has more than one view (for example in stereo), a **view**
menu appears in this row. :kbd:`;` and :kbd:`'` switch to the previous and
next view.


Color picker and information bar
--------------------------------

The information bar under the image shows, from left to right:

- the input (**A** or **B**);
- the pixel components and bit depth of the displayed image, for example
  ``RGBA32f``;
- the format of the image;
- its region of definition (*RoD*), where the image has data;
- during playback, the frame rate actually reached (green if it keeps up,
  orange or red if it is too slow);
- the image coordinates of the mouse pointer;
- the RGBA values of the image under the pointer, a color swatch, and the
  same color as hue, saturation, value and luminance (HSVL).

The values are those of the image computed by the nodes: gain and gamma are
not applied. With the default preferences they are linear; if you turn off
*Linear color pickers* in the preferences, they are converted with the Viewer
color process. A ``~`` before the values means that they come from a
lower-resolution version of the image (because of the zoom level or proxy
mode), so they may differ slightly from the full-resolution values. A ``?``
means that the image is not available: refresh the Viewer.

To keep a value displayed while the mouse moves elsewhere, use the color
picker:

- :kbd:`Ctrl` + left-click picks the color of one pixel (hold and drag to
  update it);
- :kbd:`Ctrl+Shift` + left-drag picks the average color of a rectangle;
- a left-click without modifier turns the picker off.

The color picker also sets color parameters: in the properties panel of a
node, turn on the picker button of a color parameter, then pick in the
Viewer.

The color picker button at the right end of the second row shows or hides
the information bar. Unchecking it also turns off the color picker.


Player and timeline
-------------------

The player controls and the timeline at the bottom of the Viewer set the
current frame of the project.

================================  ======================================
Play backward / stop / forward    :kbd:`J` / :kbd:`K` / :kbd:`L`
Previous / next frame             :kbd:`Left` / :kbd:`Right`
Back / forward by the increment   :kbd:`Shift+Left` / :kbd:`Shift+Right`
Previous / next keyframe          :kbd:`Ctrl+Shift+Left` /
                                  :kbd:`Ctrl+Shift+Right`
First / last frame                :kbd:`Ctrl+Left` / :kbd:`Ctrl+Right`
Set playback in / out point       :kbd:`Alt+I` / :kbd:`Alt+O`
================================  ======================================

The other controls of the player are:

**Playback in and out points**
   The two boxes at the ends of the player set the range that is played.
   The buttons next to them set the in or out point to the current frame.

**Current frame**
   The box between the play buttons. Type a frame number to go there.

**Frame increment**
   The number of frames that the increment buttons (and
   :kbd:`Shift+Left`/:kbd:`Shift+Right`) step by; 10 by default.

**fps**
   The playback frame rate. When the checkbox on its left is unchecked, the
   frame rate is taken from the A input; check it to type your own value.

**TC / TF**
   Displays time as timecode or as frame numbers.

**Turbo mode**
   During playback, only the Viewer is redrawn, so that the rest of the
   interface does not slow it down. The *Auto-turbo* preference turns it on
   for every playback.

**Playback mode**
   What happens at the end of the range: loop, bounce (play back and forth),
   or stop.

**Sync**
   The lock button synchronizes the frame range of the timeline with the
   Dope Sheet and the Curve Editor.

Click or drag in the timeline to change the current frame. Frames that are
already rendered are marked with a line under the timeline: light green for
frames in the RAM cache, dark green for frames in the disk cache (the colors
can be changed in the preferences). The first time a frame is played it has
to be rendered, so playback may be slower than real-time; frames played from
the cache are faster.

While the Viewer is rendering, its **Refresh** button is highlighted (see
also :doc:`getstarted-environment-progressbar`).


Other actions
-------------

The right-click menu of the image area also contains:

- **Show/Hide Overlays** (:kbd:`O`): hides the handles drawn by the nodes
  (transform handles, roto shapes, ...). "Overlays off" is then written in
  the Viewer;
- **Toggle Wipe**, **Center Wipe on Mouse**, **Previous Layer**,
  **Next Layer** and **Switch Input A and B**, described above;
- the **Show/Hide** submenu, to hide or show the player, the timeline, the
  information bar and the left, right and top toolbars, or all of them.
