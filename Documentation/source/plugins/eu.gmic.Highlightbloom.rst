.. _eu.gmic.Highlightbloom:

G’MIC Highlight bloom node
==========================

*This documentation is for version 0.3 of G’MIC Highlight bloom.*

Description
-----------

Author: David Tschumperle. Latest update: 2016/24/10.

This effect has been inspired by:

This tutorial by Sebastien Guyader and Patrick David: https://pixls.us/articles/highlight-bloom-and-photoillustration-look/

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

+----------------------------------------------+---------+---------+--------------------------------+
| Parameter / script name                      | Type    | Default | Function                       |
+==============================================+=========+=========+================================+
| Details strength (%) / ``Details_strength_`` | Double  | 90      |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Details scale / ``Details_scale``            | Double  | 60      |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Smoothness / ``Smoothness``                  | Double  | 60      |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Highlight (%) / ``Highlight_``               | Integer | 30      |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Contrast (%) / ``Contrast_``                 | Double  | 20      |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Preview type / ``Preview_type``              | Choice  | Full    | |                              |
|                                              |         |         | | **Full**                     |
|                                              |         |         | | **Forward horizontal**       |
|                                              |         |         | | **Forward vertical**         |
|                                              |         |         | | **Backward horizontal**      |
|                                              |         |         | | **Backward vertical**        |
|                                              |         |         | | **Duplicate top**            |
|                                              |         |         | | **Duplicate left**           |
|                                              |         |         | | **Duplicate bottom**         |
|                                              |         |         | | **Duplicate right**          |
|                                              |         |         | | **Duplicate horizontal**     |
|                                              |         |         | | **Duplicate vertical**       |
|                                              |         |         | | **Checkered**                |
|                                              |         |         | | **Checkered inverse)**       |
|                                              |         |         | | **Preview split = point(50** |
|                                              |         |         | | **50**                       |
|                                              |         |         | | **0**                        |
|                                              |         |         | | **0**                        |
|                                              |         |         | | **200**                      |
|                                              |         |         | | **200**                      |
|                                              |         |         | | **200**                      |
|                                              |         |         | | **0**                        |
|                                              |         |         | | **10**                       |
|                                              |         |         | | **0**                        |
+----------------------------------------------+---------+---------+--------------------------------+
| Output Layer / ``Output_Layer``              | Choice  | Layer 0 | |                              |
|                                              |         |         | | **Merged**                   |
|                                              |         |         | | **Layer 0**                  |
|                                              |         |         | | **Layer 1**                  |
|                                              |         |         | | **Layer 2**                  |
|                                              |         |         | | **Layer 3**                  |
|                                              |         |         | | **Layer 4**                  |
|                                              |         |         | | **Layer 5**                  |
|                                              |         |         | | **Layer 6**                  |
|                                              |         |         | | **Layer 7**                  |
|                                              |         |         | | **Layer 8**                  |
|                                              |         |         | | **Layer 9**                  |
+----------------------------------------------+---------+---------+--------------------------------+
| Resize Mode / ``Resize_Mode``                | Choice  | Dynamic | |                              |
|                                              |         |         | | **Fixed (Inplace)**          |
|                                              |         |         | | **Dynamic**                  |
|                                              |         |         | | **Downsample 1/2**           |
|                                              |         |         | | **Downsample 1/4**           |
|                                              |         |         | | **Downsample 1/8**           |
|                                              |         |         | | **Downsample 1/16**          |
+----------------------------------------------+---------+---------+--------------------------------+
| Ignore Alpha / ``Ignore_Alpha``              | Boolean | Off     |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``   | Boolean | Off     |                                |
+----------------------------------------------+---------+---------+--------------------------------+
| Log Verbosity / ``Log_Verbosity``            | Choice  | Off     | |                              |
|                                              |         |         | | **Off**                      |
|                                              |         |         | | **Level 1**                  |
|                                              |         |         | | **Level 2**                  |
|                                              |         |         | | **Level 3**                  |
+----------------------------------------------+---------+---------+--------------------------------+
