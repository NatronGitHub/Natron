.. _eu.gmic.Instantcollagepro:

G’MIC Instant collage pro node
==============================

*This documentation is for version 0.3 of G’MIC Instant collage pro.*

Description
-----------

Note: This filter will create a collage of all available Instant [pro film emulation presets

to show you how those presets modify the look of your image. It will also download all corresponding color profiles

(this may take some time at the first run).

Author: David Tschumperle. Latest update: 2013/28/10.

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
| Image size / ``Image_size``                   | Double  | 512     |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Columns for collage / ``Columns_for_collage`` | Integer | 4       |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Label size / ``Label_size``                   | Integer | 16      |                       |
+-----------------------------------------------+---------+---------+-----------------------+
| Output as / ``Output_as``                     | Choice  | Table   | |                     |
|                                               |         |         | | **Table**           |
|                                               |         |         | | **Multiple layers** |
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
