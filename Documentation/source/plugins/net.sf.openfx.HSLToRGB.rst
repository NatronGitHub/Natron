.. _net.sf.openfx.HSLToRGB:

HSLToRGB
========

*This documentation is for version 1.0 of HSLToRGB.*

Convert from HSL color model (hue, saturation, lightness, as defined by Joblove and Greenberg in 1978) to linear RGB. H is in degrees, S and L are in the same units as RGB. RGB is gamma-decompressed using the sRGB Electro-Optical Transfer Function (EOTF) after conversion.

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
