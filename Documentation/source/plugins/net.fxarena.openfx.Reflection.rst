.. _net.fxarena.openfx.Reflection:

ReflectionOFX
=============

.. figure:: net.fxarena.openfx.Reflection.png
   :alt: 

*This documentation is for version 3.2 of ReflectionOFX.*

Mirror/Reflection tranform node.

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

+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Label (UI Name)      | Script-Name   | Type      | Default-Value   | Function                                                                                                |
+======================+===============+===========+=================+=========================================================================================================+
| Reflection offset    | offset        | Integer   | 0               | Reflection offset                                                                                       |
+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Reflection spacing   | spacing       | Integer   | 0               | Space between image and reflection                                                                      |
+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Reflection           | reflection    | Boolean   | On              | Apply reflection                                                                                        |
+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Matte                | matte         | Boolean   | Off             | Merge Alpha before applying effect                                                                      |
+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| Mirror               | mirror        | Choice    | Undefined       | Select mirror type                                                                                      |
+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
| OpenMP               | openmp        | Boolean   | Off             | Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host.   |
+----------------------+---------------+-----------+-----------------+---------------------------------------------------------------------------------------------------------+
