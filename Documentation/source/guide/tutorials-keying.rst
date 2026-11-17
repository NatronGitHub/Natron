.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Keying
======

These tutorials show practical techniques for pulling keys (mattes) from green
screen and blue screen footage, and for removing the colour spill of the screen
from the foreground.

- :doc:`Hue Keyer <tutorials-keying-huekeyer>`: build a mask from a range of
  hues, and combine it with HueCorrect for colour effects.
- :doc:`Multipass keying <tutorials-keying-multipass>`: chain several
  ChromaKeyer nodes to build a clean core matte and soft edges.
- :doc:`Green screen despilling tips <tutorials-keying-despill>`: use the
  spill maps of the despill nodes as an extra matte.
- :doc:`Refining a matte with ColorLookup <tutorials-keying-colorlookup>`:
  use curves instead of the keyer's own controls to adjust the matte contrast.

See also the :doc:`alternative matte extraction tutorial
<tutorials-AlternativeMatteExtraction>`, which shows how to turn the spill maps
of Despill, lp_ChillSpill and ColorSuppression into mattes.

.. toctree::
   :maxdepth: 2

   tutorials-keying-huekeyer
   tutorials-keying-multipass
   tutorials-keying-despill
   tutorials-keying-colorlookup
