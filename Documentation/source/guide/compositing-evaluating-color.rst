.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Evaluating colors and values
============================

A monitor can only show values between 0 and 1, and the eye adapts to what
it sees, so judging an image only by looking at it is unreliable. Natron has
several tools to check the actual values of an image: dark areas that are not
quite black, highlights above 1, negative values, a matte that is not quite
opaque, or two plates that do not match.

None of these tools change the rendered images, except the nodes described
below, which you should only connect temporarily, or only to a Viewer.

In the Viewer
-------------

Pixel values
   The information bar under the image shows the RGBA values of the pixel
   under the mouse pointer. :kbd:`Ctrl` + click picks one pixel and keeps
   its value displayed, and :kbd:`Ctrl+Shift` + drag picks the average over
   a rectangle. Prefer an average to a single pixel: grain and noise make
   single-pixel values unreliable. By default the values are linear, the
   values seen by the nodes, without the Viewer gain, gamma or color
   process. See :doc:`getstarted-environment-viewer`.

Gain and gamma
   The **Gain** (in f-stops) and **Gamma** sliders of the Viewer change the
   display only. Use them to stress-test an image: a high gain reveals what
   hides in the dark areas (noise, a gray haze in what should be black,
   holes in the black of a matte), and a low gain or gamma shows what
   happens above 1, for example whether highlights are clipped or still
   hold detail. Click the buttons next to the sliders to go back to the
   neutral values, so that you do not judge the next shot with a modified
   display.

Auto-contrast
   For images whose values are far outside 0 to 1, such as depth or motion
   vectors, **Auto-contrast** stretches the visible range of values to the
   display.

Channels
   Look at each channel alone (:kbd:`R`, :kbd:`G`, :kbd:`B`, :kbd:`A`,
   :kbd:`Y` for luminance). Noise and compression artifacts are often much
   stronger in one channel, usually blue.

Comparing two images
   Connect two nodes to the **A** and **B** inputs of the Viewer and use a
   wipe to compare them, for example a plate and its graded version, or a
   foreground and the background it must match.

The histogram
-------------

A histogram shows how many pixels have each value. Open one with **New
histogram** in the menu of the button at the top left of any pane. Its
right-click menu selects the Viewer and input it reads from, the channels to
display (**RGB**, **A**, **Y**, **R**, **G** or **B**), and whether it uses
the visible part of the image or the **Full image**. The histogram is useful
to compare the distribution of two images, and to see where the darkest and
brightest values of a plate are.

Nodes for checking values
-------------------------

:ref:`ClipTest <net.sf.openfx.ClipTestPlugin>` (**Color > Math** menu)
   Draws zebra stripes over all the pixels below **Lower** or above
   **Upper** (0 and 1 by default). Put it just before a Viewer to see which
   areas are clipped, or have negative values, for example after a
   grade. Do not connect it to the rest of the graph.

:ref:`Clamp <net.sf.openfx.Clamp>` (**Color** menu)
   Clamps values to a **Minimum** and a **Maximum**. With **Enable
   MinClampTo** and **Enable MaxClampTo**, the clamped pixels are replaced by
   the **MinClampTo** and **MaxClampTo** colors instead, so that you can
   paint them in a bright color to make them visible. Clamp is also a normal
   processing node: clamping the alpha between 0 and 1 is often a good idea
   after a keyer or a merge.

:ref:`ImageStatistics <net.sf.openfx.ImageStatistics>` (**Other** menu)
   Computes the minimum, maximum, mean, standard deviation and other
   statistics of the image, over a rectangle drawn in the Viewer or over the
   whole image (uncheck **Restrict to Rectangle**), for the current frame
   (**Analyze Frame**) or for every frame (**Analyze Sequence**, which
   animates the results). It also finds the position and color of the
   darkest and brightest pixels (**Min Luma Pixel** and **Max Luma Pixel**).
   Use it to measure a value
   precisely, for example the black level of a plate, or to remove flicker:
   analyze the darkest and brightest pixels of the sequence, and use their
   values as the **Black Point** and **White Point** of a Grade node.

:ref:`MatteMonitor <net.sf.openfx.MatteMonitorPlugin>` (**Keyer** menu)
   Makes alpha values that are almost, but not exactly, 0 or 1 clearly
   visible, to check a matte. See :doc:`compositing-keying`.

:ref:`Merge <net.sf.openfx.MergePlugin>` with the **difference** operation
   Shows the absolute difference between A and B. A black result means the
   images are identical. Use it to check that a change has no effect where it
   should not, or to see how two frames or two renders differ. Raise the
   Viewer gain to see small differences.

Further reading
---------------

`Evaluating Color
<https://web.archive.org/web/20220627024754/http://www.opticalenquiry.com/nuke/index.php?title=Evaluating_Color>`__,
on the archived Optical Enquiry Nuke wiki, describes the equivalent tools of
Nuke: the Pixel Analyzer and the scopes are panels there, and Nuke's CurveTool
does what the ImageStatistics node does in Natron.

See also
--------

- :doc:`tutorials-hsvtool`: picking a source color and adjusting HSVTool
  ranges while viewing the alpha channel.
