.. _net.sf.openfx.Mirror:

MirrorOFX
=========

.. figure:: net.sf.openfx.Mirror.png
   :alt: 

*This documentation is for version 1.0 of MirrorOFX.*

Flip (vertical mirror) or flop (horizontal mirror) an image. Interlaced video can not be flipped.

This plugin does not concatenate transforms.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+---------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------+
| Label (UI Name)     | Script-Name   | Type      | Default-Value   | Function                                                                       |
+=====================+===============+===========+=================+================================================================================+
| Vertical (flip)     | flip          | Boolean   | Off             | Upside-down (swap top and bottom). Only possible if input is not interlaced.   |
+---------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------+
| Horizontal (flop)   | flop          | Boolean   | Off             | Mirror image (swap left and right)                                             |
+---------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------+
