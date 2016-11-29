.. _net.sf.openfx.RGBToHSL:

RGBToHSL
========

*This documentation is for version 1.0 of RGBToHSL.*

Convert from RGB to HSL color model (hue, saturation, lightness, as defined by Joblove and Greenberg in 1978). H is in degrees, S and L are in the same units as RGB. RGB is gamma-compressed using the sRGB Opto-Electronic Transfer Function (OETF) before conversion.

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
