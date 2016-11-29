.. _net.sf.openfx.RGBToYPbPr601:

RGBToYPbPr601
=============

*This documentation is for version 1.0 of RGBToYPbPr601.*

Convert from RGB to YPbPr color model (ITU.BT-601). RGB is gamma-compressed using the sRGB Opto-Electronic Transfer Function (OETF) before conversion.

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
