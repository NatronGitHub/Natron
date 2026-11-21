.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Keying
======

*Keying* (or *pulling a key*) computes a matte from the colors of an image,
usually to separate a foreground from a green or blue screen, so that it can
be placed over a new background. A matte can also be drawn by hand with a
Roto node (see :doc:`compositing-rotopaint`). In practice the two are
combined: the keyer handles the soft edges, hair or motion blur, and roto
shapes handle the areas that are easy to separate.

This page gives an overview of the keyers available in Natron and general
advice. The :doc:`keying tutorials <tutorials-keying>` show complete
examples.

The keyer nodes
---------------

The **Keyer** menu of the toolbar contains:

:ref:`Keyer <net.sf.openfx.KeyerPlugin>`
   Simple keys computed from one value per pixel: the luminance, the
   similarity to a color, or a "screen" key (the key color minus the other
   components). A luminance key is useful for skies, which are often the
   brightest part of an outdoor image.

:ref:`ChromaKeyer <net.sf.openfx.ChromaKeyerPlugin>`
   A key based on the chrominance (hue and saturation) of the key color,
   with built-in suppression of the screen color. A good general-purpose
   green screen or blue screen keyer, which can be chained to build a
   matte in several passes (see :doc:`tutorials-keying-multipass`).

:ref:`PIK <net.sf.openfx.PIK>` and :ref:`PIKColor <fr.inria.PIKColor>`
   A per-pixel keyer that compares the image with a *clean plate*, that is,
   the screen without the foreground. If no clean plate was shot, PIKColor
   builds one from the footage. Because the screen color can vary across the
   image, this works well on unevenly lit screens. PIK is similar to the IBK
   keyer of Nuke.

:ref:`HueKeyer <net.sf.openfx.HueKeyer>`
   A key on a range of hues, set with curves (see
   :doc:`tutorials-keying-huekeyer`).

:ref:`Difference <net.sf.openfx.DifferencePlugin>`
   A rough matte from the difference between the shot and a clean plate.
   It needs a clean plate taken with the same camera position and lighting,
   and usually needs a lot of cleanup, but it does not need a colored screen.

:ref:`Despill <net.sf.openfx.Despill>`
   Removes the color cast of the screen on the foreground (see below).

:ref:`MatteMonitor <net.sf.openfx.MatteMonitorPlugin>`
   A viewing aid: it makes the alpha values that are almost, but not exactly,
   0 or 1 clearly visible. Use it only to look at a matte, never in the
   composite itself.

Inputs and outputs of the keyers
--------------------------------

Keyer, ChromaKeyer and PIK have two optional mask inputs:

- **InM**, the *inside mask* (also called *holdout* or *core* matte):
  areas that are certainly foreground;
- **OutM**, the *outside mask* (or *garbage* matte): areas that are certainly
  background, such as lighting rigs or the edges of the screen. It has
  priority over the inside mask.

Draw these masks with Roto nodes, loosely: the keyer only needs to handle the
area between them. This is faster than fighting the keyer's parameters to
remove a stand or a tracking marker, and it keeps the keyer settings gentle,
so the edges stay soft.

The **Output Mode** parameter selects what the keyer outputs. *Premultiplied*
gives the foreground ready to be merged. *Unpremultiplied* and *Intermediate*
(the default of Keyer) give the colors without multiplying them by the key,
so a Premult node is needed before merging. *Composite* merges the
foreground over the **Bg** input. Avoid using that composite in the final
image: the foreground nearly always needs corrections (color, grain, edges)
before it is merged, and they must be done between the keyer and the Merge
node. See :doc:`compositing-channels` about premultiplication.

General advice
--------------

**Do not color correct the plate before keying it.**
   A grade or a saturation change before the keyer changes what the keyer
   sees, and usually makes the key worse. Correct the foreground after the
   key, on the unpremultiplied colors.

**Reduce the noise first.**
   Noise and compression artifacts, often strongest in the blue channel,
   end up in the matte. Key a denoised copy of the plate (for example with
   :ref:`DenoiseSharpen <net.sf.openfx.DenoiseSharpen>`), but keep the
   original plate for the colors of the composite. PIK has a dedicated
   **PFg** input for the denoised plate; with the other keyers, copy the
   matte of the denoised plate into the alpha of the original one with a
   Shuffle or Merge node.

**A keyer works on the whole image.**
   Pushing the settings to clean up one area also changes all the others. If
   one part of the image needs different settings, key it separately, limit
   each key with a Roto mask, and combine the mattes.

**Keep the edges soft.**
   The quality of a key is in its edges. A matte that is too hard looks cut
   out. Do not blur a matte to soften it, which only makes the edges wider
   and blurrier: adjust the keyer, or combine a soft matte with a hard one
   (see below).

**Check the matte, not only the composite.**
   Look at the alpha in the Viewer (:kbd:`A`). Raise the Viewer **Gain** to
   look for gray values in what should be black, and lower the **Gamma** to
   look for holes in what should be white. Put the foreground over a plain
   bright or dark background, such as a Constant node, to reveal edge
   problems. MatteMonitor shows the remaining semi-transparent pixels.

Combining mattes
----------------

A single key is rarely good everywhere. Combine several mattes with a Merge
node, using these operations (see :doc:`compositing-merge`):

- **max** keeps the brightest value of both mattes: use it to add a holdout
  shape or a second key to a matte;
- **min** keeps the darkest value: use it to remove a garbage area from a
  matte, with the garbage area in black (a garbage shape drawn in white can
  be inverted first);
- **stencil**, B × (1 - a), removes the shape in input A from input B
  directly;
- **multiply** also intersects two mattes, but darkens their soft edges.

To change only the alpha, uncheck **R**, **G** and **B** in the output row of
the Merge node, and connect the image whose colors you want to keep to the
**B** input.

A common technique combines a *soft* key, with good edges but a noisy
inside, and a *hard* key, clean inside and outside but with edges that are too
sharp:

1. Shrink the hard matte with an :ref:`Erode <net.sf.cimg.CImgErode>` or
   :ref:`ErodeBlur <eu.cimg.ErodeBlur>` node (**Filter** menu), so that it
   stays inside the edges, and merge it into the soft matte with **max**:
   this fills the inside.
2. Grow another copy of the hard matte (a negative **Size** in ErodeBlur, or
   a :ref:`Dilate <net.sf.cimg.CImgDilate>` node), and merge it into the
   result with **min**: this cleans the outside.

The edges then come from the soft key only. Check the result in the alpha
channel: the amounts of erosion and dilation depend on the shot.

Spill
-----

Light reflected by the screen tints the edges of the foreground, and
sometimes all of it, with the screen color. ChromaKeyer and Keyer (in Screen
mode) suppress some of it. For more control, use a separate node on the
original plate and apply the matte afterwards:
:ref:`Despill <net.sf.openfx.Despill>` (**Keyer** menu),
:ref:`ColorSuppress <net.sf.openfx.ColorSuppress>` or
:ref:`HueCorrect <net.sf.openfx.HueCorrect>` (**Color** menu), which lowers
the saturation or the green channel of a range of hues. See
:doc:`tutorials-keying-despill`.

Integrating the foreground
--------------------------

Even with a good matte, a foreground merged over a new background can look
pasted on. Two nodes help along the edges:
:ref:`EdgeBlur <fr.inria.EdgeBlur>` (**Filter** menu) blurs the image only
around the edges of the matte, which imitates the slight softness of real
edges, and :ref:`LightWrap <fr.inria.LightWrap>` (**Draw** menu) spreads the
light of the background over the edges of the foreground. Use either one with
small values.

Further reading
---------------

These archived pages of the Optical Enquiry Nuke wiki use Nuke node names,
but most of the advice applies to Natron:

- `The Keyer Nodes
  <https://web.archive.org/web/20220524000628/http://www.opticalenquiry.com/nuke/index.php?title=The_Keyer_Nodes>`__
- `Keying Tips
  <https://web.archive.org/web/20211023071843/http://opticalenquiry.com/nuke/index.php?title=Keying_Tips>`__
- `Combining Alphas
  <https://web.archive.org/web/20200813104439/http://www.opticalenquiry.com/nuke/index.php?title=Combining_Alphas>`__

See also
--------

- :doc:`tutorials-keying`: step-by-step keying and despill tutorials.
- :doc:`tutorials-hsvtool`: using HSVTool as a keyer, from a range of hues,
  saturations and brightnesses.
