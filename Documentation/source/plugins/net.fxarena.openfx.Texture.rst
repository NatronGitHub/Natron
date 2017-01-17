.. _net.fxarena.openfx.Texture:

Texture node
============

|pluginIcon| 

*This documentation is for version 3.7 of Texture.*

Description
-----------

Texture/Background generator node.

Powered by ImageMagick 6.9.7-4 Q32 x86\_64 2017-01-17 http://www.imagemagick.org

ImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.

ImageMagick is distributed under the Apache 2.0 license.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | Yes        |
+----------+---------------+------------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name       | Type      | Default        | Function                                                                                                                                      |
+===============================+===========+================+===============================================================================================================================================+
| Background / ``background``   | Choice    | Misc/Stripes   | Background type                                                                                                                               |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Seed / ``seed``               | Integer   | 0              | Seed the random generator                                                                                                                     |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Width / ``width``             | Integer   | 0              | Set canvas width, default (0) is project format                                                                                               |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Height / ``height``           | Integer   | 0              | Set canvas height, default (0) is project format                                                                                              |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Color from / ``fromColor``    | String    |                | Set start color, you must set a end color for this to work. Valid values are: none (transparent), color name (red, blue etc) or hex colors    |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| Color to / ``toColor``        | String    |                | Set end color, you must set a start color for this to work. Valid values are : none (transparent), color name (red, blue etc) or hex colors   |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+
| OpenMP / ``openmp``           | Boolean   | Off            | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.                                         |
+-------------------------------+-----------+----------------+-----------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.fxarena.openfx.Texture.png
   :width: 10.0%
