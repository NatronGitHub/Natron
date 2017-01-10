.. _net.fxarena.openfx.Edges:

Edges node
==========

|pluginIcon| 

*This documentation is for version 2.0 of Edges.*

Edge extraction node.

Powered by ImageMagick 6.9.6-6 Q32 x86\_64 2017-01-04 http://www.imagemagick.org

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

+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Parameter / script name       | Type      | Default         | Function                                                                                                |
+===============================+===========+=================+=========================================================================================================+
| Width / ``width``             | Double    | 2               | Width of edges                                                                                          |
+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Brightness / ``brightness``   | Double    | 5               | Adjust edge brightness                                                                                  |
+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Smoothing / ``smoothing``     | Double    | 1               | Adjust edge smoothing                                                                                   |
+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Grayscale / ``gray``          | Boolean   | Off             | Convert to grayscale before effect                                                                      |
+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Kernel / ``kernel``           | Choice    | DiamondKernel   | Convolution Kernel                                                                                      |
+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenMP / ``openmp``           | Boolean   | Off             | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+-------------------------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.fxarena.openfx.Edges.png
   :width: 10.0%
