.. _net.fxarena.openfx.Arc:

Arc node
========

|pluginIcon| 

*This documentation is for version 4.2 of Arc.*

Description
-----------

Arc Distort transform node.

Powered by ImageMagick 6.9.7-4 Q32 x86\_64 2017-01-17 http://www.imagemagick.org

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

+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Parameter / script name      | Type      | Default       | Function                                                                                                |
+==============================+===========+===============+=========================================================================================================+
| Angle / ``angle``            | Double    | 60            | Arc angle                                                                                               |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Rotate / ``rotate``          | Double    | 0             | Arc rotate                                                                                              |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Top radius / ``top``         | Double    | 0             | Arc top radius                                                                                          |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Bottom radius / ``bottom``   | Double    | 0             | Arc bottom radius                                                                                       |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Flip / ``flip``              | Boolean   | Off           | Flip image                                                                                              |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Matte / ``matte``            | Boolean   | Off           | Merge Alpha before applying effect                                                                      |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| Virtual Pixel / ``pixel``    | Choice    | Transparent   | Virtual Pixel Method                                                                                    |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+
| OpenMP / ``openmp``          | Boolean   | Off           | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+------------------------------+-----------+---------------+---------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.fxarena.openfx.Arc.png
   :width: 10.0%
