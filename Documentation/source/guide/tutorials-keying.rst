.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Keying
======

These tutorials show practical techniques for pulling keys (mattes) from green
screen and blue screen footage, and for removing the color spill of the screen
from the foreground.

- :doc:`Hue Keyer <tutorials-keying-huekeyer>`: build a mask from a range of
  hues, and combine it with HueCorrect for color effects.
- :doc:`Multipass keying <tutorials-keying-multipass>`: chain several
  ChromaKeyer nodes to build a clean core matte and soft edges.
- :doc:`Green screen despilling tips <tutorials-keying-despill>`: use the
  spill maps of the despill nodes as an extra matte.
- :doc:`Refining a matte with ColorLookup <tutorials-keying-colorlookup>`:
  use curves instead of the keyer's own controls to adjust the matte contrast.

See also the :doc:`alternative matte extraction tutorial
<tutorials-AlternativeMatteExtraction>`, which shows how to turn the spill maps
of Despill, lp_ChillSpill and ColorSuppress into mattes.

.. toctree::
   :maxdepth: 2
   :hidden:

   tutorials-keying-huekeyer
   tutorials-keying-multipass
   tutorials-keying-despill
   tutorials-keying-colorlookup

See also
--------

- :doc:`compositing-keying`: the keyer nodes, combining mattes, and removing
  spill.
- :doc:`compositing-channels`: alpha, mattes and premultiplication.
