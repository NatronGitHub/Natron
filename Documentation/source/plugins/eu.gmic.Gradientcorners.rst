.. _eu.gmic.Gradientcorners:

G’MIC Gradient corners node
===========================

*This documentation is for version 1.0 of G’MIC Gradient corners.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

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

+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Parameter / script name                                        | Type    | Default             | Function              |
+================================================================+=========+=====================+=======================+
| Color 1 (up/left corner) / ``Color_1_upleft_corner``           | Color   | r: 1 g: 1 b: 1 a: 1 |                       |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Color 2 (up/right corner) / ``Color_2_upright_corner``         | Color   | r: 1 g: 0 b: 0 a: 0 |                       |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Color 3 (bottom/left corner) / ``Color_3_bottomleft_corner``   | Color   | r: 0 g: 1 b: 0 a: 0 |                       |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Color 4 (bottom/right corner) / ``Color_4_bottomright_corner`` | Color   | r: 0 g: 0 b: 1 a: 1 |                       |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Colorspace / ``Colorspace``                                    | Choice  | Linear RGB          | |                     |
|                                                                |         |                     | | **sRGB**            |
|                                                                |         |                     | | **Linear RGB**      |
|                                                                |         |                     | | **Lab**             |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Output Layer / ``Output_Layer``                                | Choice  | Layer 0             | |                     |
|                                                                |         |                     | | **Merged**          |
|                                                                |         |                     | | **Layer 0**         |
|                                                                |         |                     | | **Layer -1**        |
|                                                                |         |                     | | **Layer -2**        |
|                                                                |         |                     | | **Layer -3**        |
|                                                                |         |                     | | **Layer -4**        |
|                                                                |         |                     | | **Layer -5**        |
|                                                                |         |                     | | **Layer -6**        |
|                                                                |         |                     | | **Layer -7**        |
|                                                                |         |                     | | **Layer -8**        |
|                                                                |         |                     | | **Layer -9**        |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Resize Mode / ``Resize_Mode``                                  | Choice  | Dynamic             | |                     |
|                                                                |         |                     | | **Fixed (Inplace)** |
|                                                                |         |                     | | **Dynamic**         |
|                                                                |         |                     | | **Downsample 1/2**  |
|                                                                |         |                     | | **Downsample 1/4**  |
|                                                                |         |                     | | **Downsample 1/8**  |
|                                                                |         |                     | | **Downsample 1/16** |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``                                | Boolean | Off                 |                       |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
| Log Verbosity / ``Log_Verbosity``                              | Choice  | Off                 | |                     |
|                                                                |         |                     | | **Off**             |
|                                                                |         |                     | | **Level 1**         |
|                                                                |         |                     | | **Level 2**         |
|                                                                |         |                     | | **Level 3**         |
+----------------------------------------------------------------+---------+---------------------+-----------------------+
