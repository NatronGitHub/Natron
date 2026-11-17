.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Green Screen Despilling Tips
============================

This tutorial uses footage filmed at the One Button Studio of the University at
Buffalo, NY. It shows how the despill nodes of Natron can produce a matte, in
addition to removing the green spill from the foreground.

Three despill nodes are available, from Natron and from the community:

- :ref:`Despill <net.sf.openfx.Despill>`, with the **Spillmap to Alpha**
  option;
- **lp_ChillSpill**, with **Shuffle Spillmatte to Alpha**;
- **DespillMadness**, with **Output Spill Matte in Alpha**.

lp_ChillSpill and DespillMadness are community PyPlugs from the
`natron-plugins <https://github.com/NatronGitHub/natron-plugins>`_
collection. The three options do the same thing: they output the spill map
(the amount of spill removed from each pixel) in the alpha channel.

The three despill nodes side by side
------------------------------------

Here, the three despill nodes are connected to the same source, each one with
its own Viewer, so that their results can be compared.

.. figure:: _images/tutorials-keying/despill-three-despillers.png
   :width: 650px
   :align: center
   :alt: Natron window with three Viewers and a node graph with Despill, lp_ChillSpill and DespillMadness side by side

   Despill, lp_ChillSpill and DespillMadness side by side.

Each node suppresses the spill a little differently, but the spill mattes they
produce are very similar. The matte depends on how the spill map is computed
from the other two channels (for example their average, their maximum or
their minimum).

Turning the spill map into a matte
----------------------------------

The raw spill map has low contrast. A :ref:`ColorLookup
<net.sf.openfx.ColorLookupPlugin>` node after the despill node can crush the
blacks and expand the whites. Here is the spill matte of lp_ChillSpill before
and after ColorLookup:

.. figure:: _images/tutorials-keying/despill-spillmatte-before.jpg
   :width: 650px
   :align: center
   :alt: Grey spill matte with soft, noisy edges

   The spill matte of lp_ChillSpill.

.. figure:: _images/tutorials-keying/despill-spillmatte-after.png
   :width: 650px
   :align: center
   :alt: Clean black and white spill matte

   The same matte after ColorLookup.

.. note::

   When you crush and expand a matte with ColorLookup, enable **Clamp Black**
   and **Clamp White** in ColorLookup, or add a :ref:`Clamp
   <net.sf.openfx.Clamp>` node after it. Otherwise steep curves can produce
   values below 0 or above 1 in the matte.

Below, the spill mattes of the three despill nodes are all adjusted with
ColorLookup. In the spill matte the screen is white and the person is black:
use an :ref:`Invert <net.sf.openfx.Invert>` node to get a matte where the
screen is black and the person is white.

.. figure:: _images/tutorials-keying/despill-mattes.png
   :width: 650px
   :align: center
   :alt: Three Viewers showing black silhouettes on white, one per despill node

   The three spill mattes after ColorLookup.

The despill nodes are used here as alternative keyers. In this example a
second despill node was still needed: the first one used the average of the
other two channels, and the footage had been converted to a highly compressed
MP4 file, which left artifacts on the edges.

This workflow doesn't replace colour-difference keyers, chroma keyers or other
dedicated keyers. It complements them, and it can help keep the edges of the
matte smooth.

Finally, build the composite and colour-correct it. In the node graph below,
the matte from Despill is inverted (:ref:`Invert <net.sf.openfx.Invert>`),
softened at the edges (:ref:`ErodeBlur <eu.cimg.ErodeBlur>`) and used as the
mask of a :ref:`Merge <net.sf.openfx.MergePlugin>`. The result is premultiplied
(:ref:`Premult <net.sf.openfx.Premult>`), despilled again by a second Despill
node, and colour-corrected (:ref:`ColorCorrect
<net.sf.openfx.ColorCorrectPlugin>`).

.. figure:: _images/tutorials-keying/despill-final.png
   :width: 650px
   :align: center
   :alt: Natron window with the final composite in the left Viewer and the node graph of the Despill branch

   The finished shot (left Viewer) and its node graph.
