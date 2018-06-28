.. _eu.gmic.Gradientfromline:

G’MIC Gradient from line node
=============================

*This documentation is for version 0.3 of G’MIC Gradient from line.*

Description
-----------

Note: Set length to 0 to release gradient length constraints.

Author: David Tschumperle. Latest update: 2015/29/06.

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com).

Inputs
------

+-------+-------------+----------+
| Input | Description | Optional |
+=======+=============+==========+
| Input |             | No       |
+-------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------------------+---------+------------+---------------------------+
| Parameter / script name                    | Type    | Default    | Function                  |
+============================================+=========+============+===========================+
| Starting point (%) / ``Starting_point_``   | Double  | 0          |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Ending point (%) / ``Ending_point_``       | Double  | 0          |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Sampling / ``Sampling``                    | Double  | 100        |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Length / ``Length``                        | Integer | 0          |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Sort colors / ``Sort_colors``              | Choice  | Don’t sort | |                         |
|                                            |         |            | | **Don’t sort**          |
|                                            |         |            | | **By red component**    |
|                                            |         |            | | **By green component**  |
|                                            |         |            | | **By blue component**   |
|                                            |         |            | | **By luminance**        |
|                                            |         |            | | **By blue chrominance** |
|                                            |         |            | | **By red chrominance**  |
|                                            |         |            | | **By lightness**        |
+--------------------------------------------+---------+------------+---------------------------+
| Reverse gradient / ``Reverse_gradient``    | Boolean | Off        |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Preview gradient / ``Preview_gradient``    | Boolean | On         |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Save gradient as / ``Save_gradient_as``    | String  |            |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0    | |                         |
|                                            |         |            | | **Merged**              |
|                                            |         |            | | **Layer 0**             |
|                                            |         |            | | **Layer 1**             |
|                                            |         |            | | **Layer 2**             |
|                                            |         |            | | **Layer 3**             |
|                                            |         |            | | **Layer 4**             |
|                                            |         |            | | **Layer 5**             |
|                                            |         |            | | **Layer 6**             |
|                                            |         |            | | **Layer 7**             |
|                                            |         |            | | **Layer 8**             |
|                                            |         |            | | **Layer 9**             |
+--------------------------------------------+---------+------------+---------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic    | |                         |
|                                            |         |            | | **Fixed (Inplace)**     |
|                                            |         |            | | **Dynamic**             |
|                                            |         |            | | **Downsample 1/2**      |
|                                            |         |            | | **Downsample 1/4**      |
|                                            |         |            | | **Downsample 1/8**      |
|                                            |         |            | | **Downsample 1/16**     |
+--------------------------------------------+---------+------------+---------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off        |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off        |                           |
+--------------------------------------------+---------+------------+---------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off        | |                         |
|                                            |         |            | | **Off**                 |
|                                            |         |            | | **Level 1**             |
|                                            |         |            | | **Level 2**             |
|                                            |         |            | | **Level 3**             |
+--------------------------------------------+---------+------------+---------------------------+
