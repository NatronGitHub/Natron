.. _eu.gmic.Tileablerotation:

G’MIC Tileable rotation node
============================

*This documentation is for version 0.3 of G’MIC Tileable rotation.*

Description
-----------

Note: This filter implements the tileable rotation technique described by Peter Yu, at:

[Peter Yu Create rotated tileable patterns: http://www.peteryu.ca/tutorials/gimp/rotate_tileable_patterns

Author: David Tschumperle. Latest update: 2011/26/05.

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

+-----------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                       | Type    | Default | Function              |
+===============================================+=========+=========+=======================+
| Angle / ``Angle``                             | Double  | 45      |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Maximum size factor / ``Maximum_size_factor`` | Integer | 8       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Array mode / ``Array_mode``                   | Choice  | None    | |                     |
|                                               |         |         | | **None**            |
|                                               |         |         | | **x-axis**          |
|                                               |         |         | | **y-axis**          |
|                                               |         |         | | **xy-axes**         |
|                                               |         |         | | **2xy-axes**        |
+-----------------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``               | Choice  | Layer 0 | |                     |
|                                               |         |         | | **Merged**          |
|                                               |         |         | | **Layer 0**         |
|                                               |         |         | | **Layer 1**         |
|                                               |         |         | | **Layer 2**         |
|                                               |         |         | | **Layer 3**         |
|                                               |         |         | | **Layer 4**         |
|                                               |         |         | | **Layer 5**         |
|                                               |         |         | | **Layer 6**         |
|                                               |         |         | | **Layer 7**         |
|                                               |         |         | | **Layer 8**         |
|                                               |         |         | | **Layer 9**         |
+-----------------------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``                 | Choice  | Dynamic | |                     |
|                                               |         |         | | **Fixed (Inplace)** |
|                                               |         |         | | **Dynamic**         |
|                                               |         |         | | **Downsample 1/2**  |
|                                               |         |         | | **Downsample 1/4**  |
|                                               |         |         | | **Downsample 1/8**  |
|                                               |         |         | | **Downsample 1/16** |
+-----------------------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``               | Boolean | Off     |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``    | Boolean | Off     |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity``             | Choice  | Off     | |                     |
|                                               |         |         | | **Off**             |
|                                               |         |         | | **Level 1**         |
|                                               |         |         | | **Level 2**         |
|                                               |         |         | | **Level 3**         |
+-----------------------------------------------+---------+---------+-----------------------+
