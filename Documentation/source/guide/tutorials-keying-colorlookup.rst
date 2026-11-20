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
   :alt: A man in front of a green screen, the noisy matte from ChromaKeyer, and the clean matte after ColorLookup

   From left to right: the original image, the matte from ChromaKeyer, and the
   matte after adjusting the alpha curve of ColorLookup.

In this example, ChromaKeyer1 has a **Key Color** of 0.15, 0.33, 0.10 (linear
RGB, picked on the screen) and an **Acceptance Angle** of 60. The screen is
not very saturated, so its matte is noisy and gray, especially on the right,
where the screen is darker. The alpha curve of ColorLookup1 has control points at (0, 0), (0.45, 0),
(0.85, 1) and (1, 1): alpha values below 0.45 become 0 and values above 0.85
become 1. **Clamp Black** and **Clamp White** are enabled. Premult1 follows
ColorLookup1.

.. figure:: _images/tutorials-keying/colorlookup-result.jpg
   :width: 400px
   :align: center
   :alt: The man, keyed, over a gray background

   The output of Premult1, over a gray background. Some green spill remains on
   the hair: see the :doc:`despilling tips <tutorials-keying-despill>`.

.. Screenshot wanted here (the previous one showed a photo that could not be
   redistributed): the properties of ColorLookup1 with the alpha curve
   selected, and the node graph Read1, ChromaKeyer1, ColorLookup1, Premult1.

.. note::

   Check every key with a high **gamma** in the Viewer: the gamma slider is
   in the second row of the Viewer toolbar, next to the gain slider. This is
   often called "slamming the gamma"; raising the gain ("slamming the gain")
   is also common. Many flaws in a matte are invisible at normal gamma,
   especially if the monitor isn't calibrated. This check is especially
   useful after adjusting a matte with ColorLookup.

See also
--------

- :doc:`compositing-keying`: the keyer nodes, and their unpremultiplied and
  premultiplied outputs.

Image credits
-------------

- Source photograph: `Peter Alsop
  <https://commons.wikimedia.org/wiki/File:Peter_Alsop.jpg>`_ by Moose School
  Productions (originally on peteralsop.com, no longer online), licensed under `CC BY 4.0
  <https://creativecommons.org/licenses/by/4.0/>`_. Resized; the mattes and
  the result were rendered from it with Natron.
