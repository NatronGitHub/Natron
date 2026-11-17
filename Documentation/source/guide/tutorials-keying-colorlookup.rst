.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Refining a Matte with ColorLookup
=================================

When you pull a key with :ref:`Keyer <net.sf.openfx.KeyerPlugin>` or
:ref:`ChromaKeyer <net.sf.openfx.ChromaKeyerPlugin>`, you can adjust the
contrast of the matte with the curves of a :ref:`ColorLookup
<net.sf.openfx.ColorLookupPlugin>` node instead of the keyer's own controls,
such as **Key Lift** and **Key Gain** in ChromaKeyer.

On some footage you have to raise the **Acceptance Angle** of ChromaKeyer more
than you'd like, or it is hard to find the right **Center** in Keyer, to get a
high-contrast matte. The edges can then suffer, depending on the hue you key.

The curves of ColorLookup give you more control to crush the blacks and expand
the whites of the matte. You can add control points to limit the adjustment to
the shadows, the midtones or the highlights. Key Lift and Key Gain work well,
but they don't give that control over the midtones, which affect the edges.

Set the **Output Mode** of the keyer to **Intermediate** (or
**Unpremultiplied**), so that the colors are not yet multiplied by the matte.
Add a ColorLookup node after the keyer, select the **alpha** curve, and adjust
it to refine the matte. Then add a :ref:`Premult <net.sf.openfx.Premult>` node
after ColorLookup, so that the colors are premultiplied by the refined matte.
If the keyer outputs a premultiplied image, its colors stay multiplied by the
old matte, which leaves noise where the new matte is transparent and wrong
colors on the edges.

.. figure:: _images/tutorials-keying/colorlookup-matte.jpg
   :width: 650px
   :align: center
   :alt: Natron window with the original image, the ChromaKeyer matte and the matte after ColorLookup, and the alpha curve of ColorLookup

   From left to right: the original image, the matte from ChromaKeyer, and the
   matte after adjusting the alpha curve of ColorLookup. In this screenshot,
   ChromaKeyer is set to Premultiplied: use Intermediate and a Premult node,
   as described above, to get correct colors.

.. note::

   Check every key with a high **gamma** in the Viewer: the gamma slider is
   in the second row of the Viewer toolbar, next to the gain slider. This is
   often called "slamming the gamma"; raising the gain ("slamming the gain")
   is also common. Many flaws in a matte are invisible at normal gamma,
   especially if the monitor isn't calibrated. This check is especially
   useful after adjusting a matte with ColorLookup.

Footage credits: the footage and the screenshot were provided by the author of
this tutorial, @blackearth2014.
