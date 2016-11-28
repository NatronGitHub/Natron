.. _net.fxarena.openfx.Polaroid:

PolaroidOFX
===========

.. figure:: net.fxarena.openfx.Polaroid.png
   :alt: 

*This documentation is for version 1.4 of PolaroidOFX.*

Polaroid image effect node.

Powered by ImageMagick 7.0.3-4 Q32 x86\_64 2016-10-23 http://www.imagemagick.org

ImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.

ImageMagick is distributed under the Apache 2.0 license.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+
| Mask     |               | Yes        |
+----------+---------------+------------+

Controls
--------

+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name    | Type      | Default-Value        | Function                                                                                                |
+===================+================+===========+======================+=========================================================================================================+
| Angle             | angle          | Double    | 5                    | Adjust polaroid angle                                                                                   |
+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
| Caption           | caption        | String    | Enter text           | Add caption to polaroid                                                                                 |
+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
| Font family       | font           | Choice    | D/DejaVu-Sans-Book   | The name of the font to be used                                                                         |
+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
| Font              | selectedFont   | String    | N/A                  | Selected font                                                                                           |
+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
| Font size         | size           | Integer   | 64                   | The height of the characters to render in pixels                                                        |
+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
| OpenMP            | openmp         | Boolean   | Off                  | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+-------------------+----------------+-----------+----------------------+---------------------------------------------------------------------------------------------------------+
