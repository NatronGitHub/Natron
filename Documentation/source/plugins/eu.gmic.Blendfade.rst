.. _eu.gmic.Blendfade:

G’MIC Blend fade node
=====================

*This documentation is for version 0.3 of G’MIC Blend fade.*

Description
-----------

The parameters below are used in most presets.

The formula below is used for the Custom preset.

Note:

This filter needs two layers to work properly. Set the Input layers option to handle multiple input layers.

Author: David Tschumperle. Latest update: 2013/21/01.

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com).

Inputs
------

+-----------+-------------+----------+
| Input     | Description | Optional |
+===========+=============+==========+
| Input     |             | No       |
+-----------+-------------+----------+
| Ext. In 1 |             | Yes      |
+-----------+-------------+----------+
| Ext. In 2 |             | Yes      |
+-----------+-------------+----------+
| Ext. In 3 |             | Yes      |
+-----------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+------------------------------------+---------+--------------------------------+-----------------------+
| Parameter / script name            | Type    | Default                        | Function              |
+====================================+=========+================================+=======================+
| Preset / ``Preset``                | Choice  | Linear                         | |                     |
|                                    |         |                                | | **Custom**          |
|                                    |         |                                | | **Linear**          |
|                                    |         |                                | | **Circular**        |
|                                    |         |                                | | **Wave**            |
|                                    |         |                                | | **Keftales**        |
+------------------------------------+---------+--------------------------------+-----------------------+
| Offset / ``Offset``                | Double  | 0                              |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Thinness / ``Thinness``            | Double  | 0                              |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Sharpness / ``Sharpness``          | Double  | 5                              |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Sharpest / ``Sharpest``            | Boolean | Off                            |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Revert layers / ``Revert_layers``  | Boolean | Off                            |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Colorspace / ``Colorspace``        | Choice  | sRGB                           | |                     |
|                                    |         |                                | | **sRGB**            |
|                                    |         |                                | | **Linear RGB**      |
|                                    |         |                                | | **Lab**             |
+------------------------------------+---------+--------------------------------+-----------------------+
| 1st parameter / ``p1st_parameter`` | Double  | 0                              |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| 2nd parameter / ``p2nd_parameter`` | Double  | 0                              |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| 3rd parameter / ``p3rd_parameter`` | Double  | 0                              |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Formula / ``Formula``              | String  | cos(4*pi*x/w) \* sin(4*pi*y/h) |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Output Layer / ``Output_Layer``    | Choice  | Layer 0                        | |                     |
|                                    |         |                                | | **Merged**          |
|                                    |         |                                | | **Layer 0**         |
|                                    |         |                                | | **Layer 1**         |
|                                    |         |                                | | **Layer 2**         |
|                                    |         |                                | | **Layer 3**         |
|                                    |         |                                | | **Layer 4**         |
|                                    |         |                                | | **Layer 5**         |
|                                    |         |                                | | **Layer 6**         |
|                                    |         |                                | | **Layer 7**         |
|                                    |         |                                | | **Layer 8**         |
|                                    |         |                                | | **Layer 9**         |
+------------------------------------+---------+--------------------------------+-----------------------+
| Resize Mode / ``Resize_Mode``      | Choice  | Dynamic                        | |                     |
|                                    |         |                                | | **Fixed (Inplace)** |
|                                    |         |                                | | **Dynamic**         |
|                                    |         |                                | | **Downsample 1/2**  |
|                                    |         |                                | | **Downsample 1/4**  |
|                                    |         |                                | | **Downsample 1/8**  |
|                                    |         |                                | | **Downsample 1/16** |
+------------------------------------+---------+--------------------------------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``    | Boolean | Off                            |                       |
+------------------------------------+---------+--------------------------------+-----------------------+
| Log Verbosity / ``Log_Verbosity``  | Choice  | Off                            | |                     |
|                                    |         |                                | | **Off**             |
|                                    |         |                                | | **Level 1**         |
|                                    |         |                                | | **Level 2**         |
|                                    |         |                                | | **Level 3**         |
+------------------------------------+---------+--------------------------------+-----------------------+
