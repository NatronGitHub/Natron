.. _eu.gmic.Contrastswissmask:

G’MIC Contrast swiss mask node
==============================

*This documentation is for version 0.3 of G’MIC Contrast swiss mask.*

Description
-----------

Merge the Mask

Author: PhotoComiX. Latest update: 2011/01/01.

Filter explained here: http://www.gimpchat.com/viewtopic.php?f=9&t=864

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

+-------------------------------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                                           | Type    | Default | Function              |
+===================================================================+=========+=========+=======================+
| Blur the mask / ``Blur_the_mask``                                 | Double  | 2       |                       |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| note / ``note``                                                   | Double  | 0       |                       |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| Skip to use the mask to boost / ``Skip_to_use_the_mask_to_boost`` | Boolean | Off     |                       |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| note_2 / ``note_2``                                               | Double  | 0       |                       |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| Intensity / ``Intensity``                                         | Double  | 1       |                       |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``                                   | Choice  | Layer 0 | |                     |
|                                                                   |         |         | | **Merged**          |
|                                                                   |         |         | | **Layer 0**         |
|                                                                   |         |         | | **Layer 1**         |
|                                                                   |         |         | | **Layer 2**         |
|                                                                   |         |         | | **Layer 3**         |
|                                                                   |         |         | | **Layer 4**         |
|                                                                   |         |         | | **Layer 5**         |
|                                                                   |         |         | | **Layer 6**         |
|                                                                   |         |         | | **Layer 7**         |
|                                                                   |         |         | | **Layer 8**         |
|                                                                   |         |         | | **Layer 9**         |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``                                     | Choice  | Dynamic | |                     |
|                                                                   |         |         | | **Fixed (Inplace)** |
|                                                                   |         |         | | **Dynamic**         |
|                                                                   |         |         | | **Downsample 1/2**  |
|                                                                   |         |         | | **Downsample 1/4**  |
|                                                                   |         |         | | **Downsample 1/8**  |
|                                                                   |         |         | | **Downsample 1/16** |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``                                   | Boolean | Off     |                       |
+-------------------------------------------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity``                                 | Choice  | Off     | |                     |
|                                                                   |         |         | | **Off**             |
|                                                                   |         |         | | **Level 1**         |
|                                                                   |         |         | | **Level 2**         |
|                                                                   |         |         | | **Level 3**         |
+-------------------------------------------------------------------+---------+---------+-----------------------+
