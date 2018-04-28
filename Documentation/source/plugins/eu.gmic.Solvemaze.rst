.. _eu.gmic.Solvemaze:

G’MIC Solve maze node
=====================

*This documentation is for version 0.3 of G’MIC Solve maze.*

Description
-----------

Author: David Tschumperle. Latest update: 2011/01/09.

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

+--------------------------------------------+---------+----------------+-----------------------+
| Parameter / script name                    | Type    | Default        | Function              |
+============================================+=========+================+=======================+
| Starting X-coord / ``Starting_Xcoord``     | Double  | 5              |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Starting Y-coord / ``Starting_Ycoord``     | Double  | 5              |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Ending X-coord / ``Ending_Xcoord``         | Double  | 95             |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Endind Y-coord / ``Endind_Ycoord``         | Double  | 95             |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Smoothness / ``Smoothness``                | Double  | 0.1            |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Thickness / ``Thickness``                  | Integer | 3              |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Color / ``Color``                          | Color   | r: 1 g: 0 b: 0 |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Maze type / ``Maze_type``                  | Choice  | Dark walls     | |                     |
|                                            |         |                | | **Dark walls**      |
|                                            |         |                | | **White walls**     |
+--------------------------------------------+---------+----------------+-----------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0        | |                     |
|                                            |         |                | | **Merged**          |
|                                            |         |                | | **Layer 0**         |
|                                            |         |                | | **Layer 1**         |
|                                            |         |                | | **Layer 2**         |
|                                            |         |                | | **Layer 3**         |
|                                            |         |                | | **Layer 4**         |
|                                            |         |                | | **Layer 5**         |
|                                            |         |                | | **Layer 6**         |
|                                            |         |                | | **Layer 7**         |
|                                            |         |                | | **Layer 8**         |
|                                            |         |                | | **Layer 9**         |
+--------------------------------------------+---------+----------------+-----------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic        | |                     |
|                                            |         |                | | **Fixed (Inplace)** |
|                                            |         |                | | **Dynamic**         |
|                                            |         |                | | **Downsample 1/2**  |
|                                            |         |                | | **Downsample 1/4**  |
|                                            |         |                | | **Downsample 1/8**  |
|                                            |         |                | | **Downsample 1/16** |
+--------------------------------------------+---------+----------------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off            |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off            |                       |
+--------------------------------------------+---------+----------------+-----------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off            | |                     |
|                                            |         |                | | **Off**             |
|                                            |         |                | | **Level 1**         |
|                                            |         |                | | **Level 2**         |
|                                            |         |                | | **Level 3**         |
+--------------------------------------------+---------+----------------+-----------------------+
