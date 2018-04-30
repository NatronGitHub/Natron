.. _eu.gmic.Extractobjects:

G’MIC Extract objects node
==========================

*This documentation is for version 0.3 of G’MIC Extract objects.*

Description
-----------

Author: David Tschumperle. Latest update: 2015/23/02.

Filter explained here: http://gimpchat.com/viewtopic.php?f=28&t=7905

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

+-------------------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                               | Type    | Default | Function              |
+=======================================================+=========+=========+=======================+
| X-background (%) / ``Xbackground_``                   | Double  | 0       |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Y-background (%) / ``Ybackground_``                   | Double  | 0       |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Select background point / ``Select_background_point`` | Boolean | Off     |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Color tolerance / ``Color_tolerance``                 | Integer | 20      |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Opacity threshold (%) / ``Opacity_threshold_``        | Integer | 50      |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Minimal area / ``Minimal_area``                       | Double  | 0.3     |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Connectivity / ``Connectivity``                       | Choice  | Low     | |                     |
|                                                       |         |         | | **Low**             |
|                                                       |         |         | | **High**            |
+-------------------------------------------------------+---------+---------+-----------------------+
| Output as / ``Output_as``                             | Choice  | Crop    | |                     |
|                                                       |         |         | | **Crop**            |
|                                                       |         |         | | **Segmentation**    |
+-------------------------------------------------------+---------+---------+-----------------------+
| Preview guides / ``Preview_guides``                   | Boolean | On      |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``                       | Choice  | Layer 0 | |                     |
|                                                       |         |         | | **Merged**          |
|                                                       |         |         | | **Layer 0**         |
|                                                       |         |         | | **Layer 1**         |
|                                                       |         |         | | **Layer 2**         |
|                                                       |         |         | | **Layer 3**         |
|                                                       |         |         | | **Layer 4**         |
|                                                       |         |         | | **Layer 5**         |
|                                                       |         |         | | **Layer 6**         |
|                                                       |         |         | | **Layer 7**         |
|                                                       |         |         | | **Layer 8**         |
|                                                       |         |         | | **Layer 9**         |
+-------------------------------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``                         | Choice  | Dynamic | |                     |
|                                                       |         |         | | **Fixed (Inplace)** |
|                                                       |         |         | | **Dynamic**         |
|                                                       |         |         | | **Downsample 1/2**  |
|                                                       |         |         | | **Downsample 1/4**  |
|                                                       |         |         | | **Downsample 1/8**  |
|                                                       |         |         | | **Downsample 1/16** |
+-------------------------------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``                       | Boolean | Off     |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``            | Boolean | Off     |                       |
+-------------------------------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity``                     | Choice  | Off     | |                     |
|                                                       |         |         | | **Off**             |
|                                                       |         |         | | **Level 1**         |
|                                                       |         |         | | **Level 2**         |
|                                                       |         |         | | **Level 3**         |
+-------------------------------------------------------+---------+---------+-----------------------+
