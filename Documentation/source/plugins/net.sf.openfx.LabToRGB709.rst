.. _net.sf.openfx.LabToRGB709:

LabToRGB709
===========

.. figure:: net.sf.openfx.LabToRGB709.png
   :alt: 

*This documentation is for version 1.0 of LabToRGB709.*

Convert from L\ *a*\ b color model to RGB (Rec.709 with D65 illuminant). L\ *a*\ b coordinates are divided by 100 for better visualization.

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
