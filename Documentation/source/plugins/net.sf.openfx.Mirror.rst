.. _net.sf.openfx.Mirror:

Mirror node
===========

|pluginIcon| 

*This documentation is for version 1.0 of Mirror.*

Description
-----------

Flip (vertical mirror) or flop (horizontal mirror) an image. Interlaced video can not be flipped.

This plugin does not concatenate transforms.

Inputs
------

====== =========== ========
Input  Description Optional
====== =========== ========
Source             No
====== =========== ========

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

============================ ======= ======= ============================================================================
Parameter / script name      Type    Default Function
============================ ======= ======= ============================================================================
Vertical (flip) / ``flip``   Boolean Off     Upside-down (swap top and bottom). Only possible if input is not interlaced.
Horizontal (flop) / ``flop`` Boolean Off     Mirror image (swap left and right)
============================ ======= ======= ============================================================================

.. |pluginIcon| image:: net.sf.openfx.Mirror.png
   :width: 10.0%
