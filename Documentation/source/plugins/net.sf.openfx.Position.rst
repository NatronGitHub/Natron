.. _net.sf.openfx.Position:

PositionOFX
===========

.. figure:: net.sf.openfx.Position.png
   :alt: 

*This documentation is for version 1.0 of PositionOFX.*

Translate an image by an integer number of pixels.

This plugin does not concatenate transforms.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                                                   |
+===================+===============+===========+=================+============================================================================================================================+
| Translate         | translate     | Double    | x: 0 y: 0       | New position of the bottom-left pixel. Rounded to the closest pixel.                                                       |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
| Interactive       | interactive   | Boolean   | Off             | When checked the image will be rendered whenever moving the overlay interact instead of when releasing the mouse button.   |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
