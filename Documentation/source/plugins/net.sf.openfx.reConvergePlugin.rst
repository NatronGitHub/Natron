.. _net.sf.openfx.reConvergePlugin:

ReConvergeOFX
=============

*This documentation is for version 1.0 of ReConvergeOFX.*

Shift convergence so that a tracked point appears at screen-depth. Horizontal disparity may be provided in the red channel of the disparity input if it has RGBA components, or the Alpha channel if it only has Alpha. If no disparity is given, only the offset is taken into account. The amount of shift in pixels is rounded to the closest integer. The ReConverge node only shifts views horizontally, not vertically.

Inputs
------

+-------------+---------------+------------+
| Input       | Description   | Optional   |
+=============+===============+============+
| Source      |               | No         |
+-------------+---------------+------------+
| Disparity   |               | Yes        |
+-------------+---------------+------------+

Controls
--------

+----------------------+-----------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)      | Script-Name     | Type      | Default-Value   | Function                                                                                                                   |
+======================+=================+===========+=================+============================================================================================================================+
| Converge Upon        | convergePoint   | Double    | x: 0.5 y: 0.5   | Position of the tracked point when the convergence is set                                                                  |
+----------------------+-----------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
| Interactive          | interactive     | Boolean   | Off             | When checked the image will be rendered whenever moving the overlay interact instead of when releasing the mouse button.   |
+----------------------+-----------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
| Convergence Offset   | offset          | Integer   | 0               | The disparity of the tracked point will be set to this                                                                     |
+----------------------+-----------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
| Mode                 | convergeMode    | Choice    | Shift Right     | Select to view to be shifted in order to set convergence                                                                   |
+----------------------+-----------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------+
