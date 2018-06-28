.. _eu.gmic.Quadtreevariations:

G’MIC Quadtree variations node
==============================

*This documentation is for version 0.3 of G’MIC Quadtree variations.*

Description
-----------

For ‘Ellipse painting’ only:

Author: David Tschumperle. Latest update: 2017/15/06.

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

+--------------------------------------------+---------+---------+--------------------------------+
| Parameter / script name                    | Type    | Default | Function                       |
+============================================+=========+=========+================================+
| Mode / ``Mode``                            | Choice  | Squares | |                              |
|                                            |         |         | | **Squares**                  |
|                                            |         |         | | **Sierpinksi design**        |
|                                            |         |         | | **Ellipse painting**         |
+--------------------------------------------+---------+---------+--------------------------------+
| Precision / ``Precision``                  | Integer | 1024    |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Homogeneity / ``Homogeneity``              | Double  | 0.5     |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Outline / ``Outline``                      | Integer | 0       |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Primary radius / ``Primary_radius``        | Double  | 3       |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Secondary radius / ``Secondary_radius``    | Double  | 1.5     |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Anisotropy / ``Anisotropy``                | Double  | 1       |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Only leafs / ``Only_leafs``                | Boolean | On      |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Preview type / ``Preview_type``            | Choice  | Full    | |                              |
|                                            |         |         | | **Full**                     |
|                                            |         |         | | **Forward horizontal**       |
|                                            |         |         | | **Forward vertical**         |
|                                            |         |         | | **Backward horizontal**      |
|                                            |         |         | | **Backward vertical**        |
|                                            |         |         | | **Duplicate top**            |
|                                            |         |         | | **Duplicate left**           |
|                                            |         |         | | **Duplicate bottom**         |
|                                            |         |         | | **Duplicate right**          |
|                                            |         |         | | **Duplicate horizontal**     |
|                                            |         |         | | **Duplicate vertical**       |
|                                            |         |         | | **Checkered**                |
|                                            |         |         | | **Checkered inverse)**       |
|                                            |         |         | | **Preview split = point(50** |
|                                            |         |         | | **50**                       |
|                                            |         |         | | **0**                        |
|                                            |         |         | | **0**                        |
|                                            |         |         | | **200**                      |
|                                            |         |         | | **200**                      |
|                                            |         |         | | **200**                      |
|                                            |         |         | | **0**                        |
|                                            |         |         | | **10**                       |
|                                            |         |         | | **0**                        |
+--------------------------------------------+---------+---------+--------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                              |
|                                            |         |         | | **Merged**                   |
|                                            |         |         | | **Layer 0**                  |
|                                            |         |         | | **Layer 1**                  |
|                                            |         |         | | **Layer 2**                  |
|                                            |         |         | | **Layer 3**                  |
|                                            |         |         | | **Layer 4**                  |
|                                            |         |         | | **Layer 5**                  |
|                                            |         |         | | **Layer 6**                  |
|                                            |         |         | | **Layer 7**                  |
|                                            |         |         | | **Layer 8**                  |
|                                            |         |         | | **Layer 9**                  |
+--------------------------------------------+---------+---------+--------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                              |
|                                            |         |         | | **Fixed (Inplace)**          |
|                                            |         |         | | **Dynamic**                  |
|                                            |         |         | | **Downsample 1/2**           |
|                                            |         |         | | **Downsample 1/4**           |
|                                            |         |         | | **Downsample 1/8**           |
|                                            |         |         | | **Downsample 1/16**          |
+--------------------------------------------+---------+---------+--------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                                |
+--------------------------------------------+---------+---------+--------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                              |
|                                            |         |         | | **Off**                      |
|                                            |         |         | | **Level 1**                  |
|                                            |         |         | | **Level 2**                  |
|                                            |         |         | | **Level 3**                  |
+--------------------------------------------+---------+---------+--------------------------------+
