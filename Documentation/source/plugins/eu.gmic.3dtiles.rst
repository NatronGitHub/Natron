.. _eu.gmic.3dtiles:

G’MIC 3d tiles node
===================

*This documentation is for version 0.3 of G’MIC 3d tiles.*

Description
-----------

Note:

This filter needs two layers to work properly. Set the Input layers option to handle multiple input layers.

Author: David Tschumperle. Latest update: 2012/13/08.

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

+-----------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                       | Type    | Default | Function              |
+===============================================+=========+=========+=======================+
| Inter-frames / ``Interframes``                | Integer | 10      |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| X-tiles / ``Xtiles``                          | Integer | 8       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Y-tiles / ``Ytiles``                          | Integer | 8       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| X-rotation / ``Xrotation``                    | String  | 1       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Y-rotation / ``Yrotation``                    | String  | 1       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Z-rotation / ``Zrotation``                    | String  | 0       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Focale / ``Focale``                           | Double  | 800     |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Enable antialiasing / ``Enable_antialiasing`` | Boolean | On      |                       |
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
