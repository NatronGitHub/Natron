.. _net.fxarena.openfx.Implode:

Implode node
============

|pluginIcon| 

*This documentation is for version 2.3 of Implode.*

Description
-----------

Implode transform node.

Powered by ImageMagick 6.9.7-5 Q32 x86\_64 2017-01-26 http://www.imagemagick.org

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

+---------------------------+-----------+-----------+---------------------------------------------------------------------------------------------------------+
| Parameter / script name   | Type      | Default   | Function                                                                                                |
+===========================+===========+===========+=========================================================================================================+
| Factor / ``factor``       | Double    | 0.5       | Implode image by factor                                                                                 |
+---------------------------+-----------+-----------+---------------------------------------------------------------------------------------------------------+
| Swirl / ``swirl``         | Double    | 0         | Swirl image by degree                                                                                   |
+---------------------------+-----------+-----------+---------------------------------------------------------------------------------------------------------+
| Matte / ``matte``         | Boolean   | Off       | Merge Alpha before applying effect                                                                      |
+---------------------------+-----------+-----------+---------------------------------------------------------------------------------------------------------+
| OpenMP / ``openmp``       | Boolean   | Off       | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+---------------------------+-----------+-----------+---------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.fxarena.openfx.Implode.png
   :width: 10.0%
