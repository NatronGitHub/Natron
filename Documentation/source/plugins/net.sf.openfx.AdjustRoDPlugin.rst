.. _net.sf.openfx.AdjustRoDPlugin:

AdjustRoD
=========

.. figure:: net.sf.openfx.AdjustRoDPlugin.png
   :alt: 

*This documentation is for version 1.0 of AdjustRoD.*

Enlarges the input image by a given amount of black and transparent pixels.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+----------+-----------------+--------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type     | Default-Value   | Function                                                                 |
+===================+===============+==========+=================+==========================================================================+
| Add Pixels        | addPixels     | Double   | w: 0 h: 0       | How many pixels to add on each side for both dimensions (width/height)   |
+-------------------+---------------+----------+-----------------+--------------------------------------------------------------------------+
