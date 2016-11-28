.. _net.fxarena.openfx.Sketch:

SketchOFX
=========

.. figure:: net.fxarena.openfx.Sketch.png
   :alt: 

*This documentation is for version 2.2 of SketchOFX.*

Sketch effect node.

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
| Radius            | radius        | Double    | 1               | Adjust radius                                                                                           |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Sigma             | sigma         | Double    | 0               | Adjust sigma                                                                                            |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Angle             | angle         | Double    | 0               | Adjust angle                                                                                            |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenMP            | openmp        | Boolean   | Off             | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
