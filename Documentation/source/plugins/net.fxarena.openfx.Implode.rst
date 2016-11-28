.. _net.fxarena.openfx.Implode:

ImplodeOFX
==========

.. figure:: net.fxarena.openfx.Implode.png
   :alt: 

*This documentation is for version 2.3 of ImplodeOFX.*

Implode transform node.

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
| Factor            | factor        | Double    | 0.5             | Implode image by factor                                                                                 |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Swirl             | swirl         | Double    | 0               | Swirl image by degree                                                                                   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Matte             | matte         | Boolean   | Off             | Merge Alpha before applying effect                                                                      |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenMP            | openmp        | Boolean   | Off             | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+-------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
