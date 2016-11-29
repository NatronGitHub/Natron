.. _net.sf.openfx.YCbCrToRGB709:

YCbCrToRGB709
=============

*This documentation is for version 1.0 of YCbCrToRGB709.*

Convert from YCbCr color model (ITU.BT-709) to linear RGB. RGB is gamma-decompressed using the Rec.709 Electro-Optical Transfer Function (EOTF) after conversion.

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
