.. _net.fxarena.openfx.Polaroid:

Polaroid node
=============

|pluginIcon| 

*This documentation is for version 1.4 of Polaroid.*

Description
-----------

Polaroid image effect node.

Powered by ImageMagick 6.9.6-6 Q32 x86\_64 2017-01-11 http://www.imagemagick.org

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

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+---------------------------+-----------+--------------+---------------------------------------------------------------------------------------------------------+
| Parameter / script name   | Type      | Default      | Function                                                                                                |
+===========================+===========+==============+=========================================================================================================+
| Angle / ``angle``         | Double    | 5            | Adjust polaroid angle                                                                                   |
+---------------------------+-----------+--------------+---------------------------------------------------------------------------------------------------------+
| Caption / ``caption``     | String    | Enter text   | Add caption to polaroid                                                                                 |
+---------------------------+-----------+--------------+---------------------------------------------------------------------------------------------------------+
| Font family / ``font``    | Choice    | A/Arial      | The name of the font to be used                                                                         |
+---------------------------+-----------+--------------+---------------------------------------------------------------------------------------------------------+
| Font size / ``size``      | Integer   | 64           | The height of the characters to render in pixels                                                        |
+---------------------------+-----------+--------------+---------------------------------------------------------------------------------------------------------+
| OpenMP / ``openmp``       | Boolean   | Off          | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+---------------------------+-----------+--------------+---------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.fxarena.openfx.Polaroid.png
   :width: 10.0%
