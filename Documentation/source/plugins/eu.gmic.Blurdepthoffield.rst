.. _eu.gmic.Blurdepthoffield:

G’MIC Blur depth-of-field node
==============================

*This documentation is for version 0.3 of G’MIC Blur depth-of-field.*

Description
-----------

Gaussian depth-of-field:

User-defined depth-of-field:

You can specify your own depth-of-field image, as a bottom layer image whose luminance encodes the depth for each pixel.

Don’t forget to modify the Input layers combo-box to make this layer active for the filter.

Author: David Tschumperle. Latest update: 2014/25/02.

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

+---------------------------------------------+---------+----------+-----------------------------------+
| Parameter / script name                     | Type    | Default  | Function                          |
+=============================================+=========+==========+===================================+
| Blur amplitude / ``Blur_amplitude``         | Double  | 3        |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Blur precision / ``Blur_precision``         | Integer | 16       |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Depth-of-field type / ``Depthoffield_type`` | Choice  | Gaussian | |                                 |
|                                             |         |          | | **Gaussian**                    |
|                                             |         |          | | **User-defined (bottom layer)** |
+---------------------------------------------+---------+----------+-----------------------------------+
| Invert blur / ``Invert_blur``               | Boolean | Off      |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Center (%) / ``Center_``                    | Double  | 0        |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| First radius / ``First_radius``             | Double  | 30       |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Second radius / ``Second_radius``           | Double  | 30       |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Angle / ``Angle``                           | Double  | 0        |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Sharpness / ``Sharpness``                   | Double  | 1        |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Preview guides / ``Preview_guides``         | Boolean | On       |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Gamma / ``Gamma``                           | Double  | 0        |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Output Layer / ``Output_Layer``             | Choice  | Layer 0  | |                                 |
|                                             |         |          | | **Merged**                      |
|                                             |         |          | | **Layer 0**                     |
|                                             |         |          | | **Layer 1**                     |
|                                             |         |          | | **Layer 2**                     |
|                                             |         |          | | **Layer 3**                     |
|                                             |         |          | | **Layer 4**                     |
|                                             |         |          | | **Layer 5**                     |
|                                             |         |          | | **Layer 6**                     |
|                                             |         |          | | **Layer 7**                     |
|                                             |         |          | | **Layer 8**                     |
|                                             |         |          | | **Layer 9**                     |
+---------------------------------------------+---------+----------+-----------------------------------+
| Resize Mode / ``Resize_Mode``               | Choice  | Dynamic  | |                                 |
|                                             |         |          | | **Fixed (Inplace)**             |
|                                             |         |          | | **Dynamic**                     |
|                                             |         |          | | **Downsample 1/2**              |
|                                             |         |          | | **Downsample 1/4**              |
|                                             |         |          | | **Downsample 1/8**              |
|                                             |         |          | | **Downsample 1/16**             |
+---------------------------------------------+---------+----------+-----------------------------------+
| Ignore Alpha / ``Ignore_Alpha``             | Boolean | Off      |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``  | Boolean | Off      |                                   |
+---------------------------------------------+---------+----------+-----------------------------------+
| Log Verbosity / ``Log_Verbosity``           | Choice  | Off      | |                                 |
|                                             |         |          | | **Off**                         |
|                                             |         |          | | **Level 1**                     |
|                                             |         |          | | **Level 2**                     |
|                                             |         |          | | **Level 3**                     |
+---------------------------------------------+---------+----------+-----------------------------------+
