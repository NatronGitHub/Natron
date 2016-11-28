.. _net.sf.openfx.RGB709ToXYZ:

RGB709ToXYZ
===========

.. figure:: net.sf.openfx.RGB709ToXYZ.png
   :alt: 

*This documentation is for version 1.0 of RGB709ToXYZ.*

Convert from RGB (Rec.709 with D65 illuminant) to XYZ color model. X, Y and Z are in the same units as RGB.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+-----------+-----------------+-------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                              |
+===================+===============+===========+=================+=======================================================================================================+
| Unpremult         | premult       | Boolean   | Off             | Divide the image by the alpha channel before processing. Use if the input images are premultiplied.   |
+-------------------+---------------+-----------+-----------------+-------------------------------------------------------------------------------------------------------+
