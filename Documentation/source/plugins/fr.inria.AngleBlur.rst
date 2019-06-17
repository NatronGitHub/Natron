.. _fr.inria.AngleBlur:

AngleBlur node
==============

*This documentation is for version 1.0 of AngleBlur.*

Description
-----------

The Angle Blur effect gives the illusion of motion in a given direction.

Inputs
------

+--------+-------------+----------+
| Input  | Description | Optional |
+========+=============+==========+
| Source |             | No       |
+--------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+---------------------------------------+--------+---------+-----------------------------------------------------------------------------------------------------+
| Parameter / script name               | Type   | Default | Function                                                                                            |
+=======================================+========+=========+=====================================================================================================+
| Convert to Group / ``convertToGroup`` | Button |         | Converts this node to a Group: the internal node-graph and the user parameters will become editable |
+---------------------------------------+--------+---------+-----------------------------------------------------------------------------------------------------+
| Angle / ``angleBlur_angle``           | Double | 0       | Determines the direction into which the image is blurred. This is an angle in degrees.              |
+---------------------------------------+--------+---------+-----------------------------------------------------------------------------------------------------+
| Distance / ``angleBlur_distance``     | Double | 0       | Determines how much the image will be blurred                                                       |
+---------------------------------------+--------+---------+-----------------------------------------------------------------------------------------------------+
