.. _eu.gmic.Marble:

G’MIC Marble node
=================

*This documentation is for version 1.0 of G’MIC Marble.*

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

+-------------------------------------+---------+---------+-----------------------+
| Parameter / script name             | Type    | Default | Function              |
+=====================================+=========+=========+=======================+
| Image weight / ``Image_weight``     | Double  | 0.5     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Pattern weight / ``Pattern_weight`` | Double  | 1       |                       |
+-------------------------------------+---------+---------+-----------------------+
| Pattern angle / ``Pattern_angle``   | Double  | 0       |                       |
+-------------------------------------+---------+---------+-----------------------+
| Amplitude / ``Amplitude``           | Double  | 0       |                       |
+-------------------------------------+---------+---------+-----------------------+
| Sharpness / ``Sharpness``           | Double  | 0.4     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Anisotropy / ``Anisotropy``         | Double  | 0.6     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Alpha / ``Alpha``                   | Double  | 0.6     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Sigma / ``Sigma``                   | Double  | 1.1     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Cut low / ``Cut_low``               | Double  | 0       |                       |
+-------------------------------------+---------+---------+-----------------------+
| Cut high / ``Cut_high``             | Double  | 100     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``     | Choice  | Layer 0 | |                     |
|                                     |         |         | | **Merged**          |
|                                     |         |         | | **Layer 0**         |
|                                     |         |         | | **Layer -1**        |
|                                     |         |         | | **Layer -2**        |
|                                     |         |         | | **Layer -3**        |
|                                     |         |         | | **Layer -4**        |
|                                     |         |         | | **Layer -5**        |
|                                     |         |         | | **Layer -6**        |
|                                     |         |         | | **Layer -7**        |
|                                     |         |         | | **Layer -8**        |
|                                     |         |         | | **Layer -9**        |
+-------------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``       | Choice  | Dynamic | |                     |
|                                     |         |         | | **Fixed (Inplace)** |
|                                     |         |         | | **Dynamic**         |
|                                     |         |         | | **Downsample 1/2**  |
|                                     |         |         | | **Downsample 1/4**  |
|                                     |         |         | | **Downsample 1/8**  |
|                                     |         |         | | **Downsample 1/16** |
+-------------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``     | Boolean | Off     |                       |
+-------------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity``   | Choice  | Off     | |                     |
|                                     |         |         | | **Off**             |
|                                     |         |         | | **Level 1**         |
|                                     |         |         | | **Level 2**         |
|                                     |         |         | | **Level 3**         |
+-------------------------------------+---------+---------+-----------------------+
