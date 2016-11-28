.. _net.fxarena.openfx.Edges:

EdgesOFX
========

.. figure:: net.fxarena.openfx.Edges.png
   :alt: 

*This documentation is for version 2.0 of EdgesOFX.*

Edge extraction node.

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

+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                                |
+===================+===============+===========+=================+=========================================================================================================+
| Width             | width         | Double    | 2               | Width of edges                                                                                          |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Brightness        | brightness    | Double    | 5               | Adjust edge brightness                                                                                  |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Smoothing         | smoothing     | Double    | 1               | Adjust edge smoothing                                                                                   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Grayscale         | gray          | Boolean   | Off             | Convert to grayscale before effect                                                                      |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Kernel            | kernel        | Choice    | DiamondKernel   | Convolution Kernel                                                                                      |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenMP            | openmp        | Boolean   | Off             | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
