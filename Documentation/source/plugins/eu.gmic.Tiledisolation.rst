.. _eu.gmic.Tiledisolation:

G’MIC Tiled isolation node
==========================

*This documentation is for version 0.3 of G’MIC Tiled isolation.*

Description
-----------

Author: David Tschumperle. Latest update: 2011/13/04.

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
| X-size / ``Xsize``                            | Double  | 10      |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Y-size / ``Ysize``                            | Double  | 10      |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| X-border / ``Xborder``                        | Double  | 5       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Y-border / ``Yborder``                        | Double  | 5       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Keep tiles square / ``Keep_tiles_square``     | Boolean | On      |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Keep borders square / ``Keep_borders_square`` | Boolean | On      |                       |
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
| Log Verbosity / ``Log_Verbosity``             | Choice  | Off     | |                     |
|                                               |         |         | | **Off**             |
|                                               |         |         | | **Level 1**         |
|                                               |         |         | | **Level 2**         |
|                                               |         |         | | **Level 3**         |
+-----------------------------------------------+---------+---------+-----------------------+
