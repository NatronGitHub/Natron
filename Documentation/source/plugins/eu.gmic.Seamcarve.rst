.. _eu.gmic.Seamcarve:

G’MIC Seamcarve node
====================

*This documentation is for version 0.3 of G’MIC Seamcarve.*

Description
-----------

Note:

You can define a transparent top layer that will help the seam-carving algorithm to preserve or force removing image structures:

- Draw areas in red to force removing them.

- Draw areas in green to preserve them.

- Don’t forget also to set the Input layers... parameter to input both layers to the filter.

Authors: Garagecoder and David Tschumperle. Latest update: 2014/02/06.

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

+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                                                 | Type    | Default | Function              |
+=========================================================================+=========+=========+=======================+
| Width (%) / ``Width_``                                                  | Double  | 85      |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Height (%) / ``Height_``                                                | Double  | 100     |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Maximal seams per iteration (%) / ``Maximal_seams_per_iteration_``      | Double  | 15      |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Use top layer as a priority mask / ``Use_top_layer_as_a_priority_mask`` | Boolean | Off     |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Antialiasing / ``Antialiasing``                                         | Boolean | On      |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``                                         | Choice  | Layer 0 | |                     |
|                                                                         |         |         | | **Merged**          |
|                                                                         |         |         | | **Layer 0**         |
|                                                                         |         |         | | **Layer 1**         |
|                                                                         |         |         | | **Layer 2**         |
|                                                                         |         |         | | **Layer 3**         |
|                                                                         |         |         | | **Layer 4**         |
|                                                                         |         |         | | **Layer 5**         |
|                                                                         |         |         | | **Layer 6**         |
|                                                                         |         |         | | **Layer 7**         |
|                                                                         |         |         | | **Layer 8**         |
|                                                                         |         |         | | **Layer 9**         |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``                                           | Choice  | Dynamic | |                     |
|                                                                         |         |         | | **Fixed (Inplace)** |
|                                                                         |         |         | | **Dynamic**         |
|                                                                         |         |         | | **Downsample 1/2**  |
|                                                                         |         |         | | **Downsample 1/4**  |
|                                                                         |         |         | | **Downsample 1/8**  |
|                                                                         |         |         | | **Downsample 1/16** |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``                                         | Boolean | Off     |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``                              | Boolean | Off     |                       |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity``                                       | Choice  | Off     | |                     |
|                                                                         |         |         | | **Off**             |
|                                                                         |         |         | | **Level 1**         |
|                                                                         |         |         | | **Level 2**         |
|                                                                         |         |         | | **Level 3**         |
+-------------------------------------------------------------------------+---------+---------+-----------------------+
