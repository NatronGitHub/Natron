.. _net.sf.openfx.RGB709ToLab:

RGB709ToLab
===========

.. figure:: net.sf.openfx.RGB709ToLab.png
   :alt: 

*This documentation is for version 1.0 of RGB709ToLab.*

Convert from RGB (Rec.709 with D65 illuminant) to L\ *a*\ b color model. L\ *a*\ b coordinates are divided by 100 for better visualization.

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
