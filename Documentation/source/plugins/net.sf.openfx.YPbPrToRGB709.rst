.. _net.sf.openfx.YPbPrToRGB709:

YPbPrToRGB709
=============

*This documentation is for version 1.0 of YPbPrToRGB709.*

Convert from YPbPr color model (ITU.BT-709) to RGB. RGB is gamma-decompressed using the Rec.709 Electro-Optical Transfer Function (EOTF) after conversion.

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
