.. _net.sf.openfx.RGBToYCbCr709:

RGBToYCbCr709
=============

*This documentation is for version 1.0 of RGBToYCbCr709.*

Convert from linear RGB to YCbCr color model (ITU.BT-709). RGB is gamma-compressed using the Rec.709 Opto-Electronic Transfer Function (OETF) before conversion.

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
