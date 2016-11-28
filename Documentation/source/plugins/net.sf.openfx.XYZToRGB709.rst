.. _net.sf.openfx.XYZToRGB709:

XYZToRGB709
===========

.. figure:: net.sf.openfx.XYZToRGB709.png
   :alt: 

*This documentation is for version 1.0 of XYZToRGB709.*

Convert from XYZ color model to RGB (Rec.709 with D65 illuminant). X, Y and Z are in the same units as RGB.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                            |
+===================+===============+===========+=================+=====================================================================================================+
| Premult           | premult       | Boolean   | Off             | Multiply the image by the alpha channel after processing. Use to get premultiplied output images.   |
+-------------------+---------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------+
