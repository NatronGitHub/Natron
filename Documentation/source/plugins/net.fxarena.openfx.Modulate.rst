.. _net.fxarena.openfx.Modulate:

ModulateOFX
===========

.. figure:: net.fxarena.openfx.Modulate.png
   :alt: 

*This documentation is for version 1.2 of ModulateOFX.*

Modulate color node.

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
| Brightness        | brightness    | Double    | 100             | Adjust brightness (%)                                                                                   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Saturation        | saturation    | Double    | 100             | Adjust saturation (%)                                                                                   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Hue               | hue           | Double    | 100             | Adjust hue (%)                                                                                          |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenMP            | openmp        | Boolean   | On              | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenCL            | opencl        | Boolean   | Off             | Enable/Disable OpenCL. This will enable the plugin to use supported GPU(s) for better performance.      |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
