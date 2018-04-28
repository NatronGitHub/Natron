.. _eu.gmic.Packsprites:

G’MIC Pack sprites node
=======================

*This documentation is for version 0.3 of G’MIC Pack sprites.*

Description
-----------

Notes: - Parameters Width and Height are considered only when No masking mode is selected.

- Set different sprites on different layers to pack multiple sprites at the same time.

Click here for a video tutorial: http://www.youtube.com/watch?v=bpg7CGH7vCM

Author: David Tschumperle. Latest update: 2013/24/06.

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

+--------------------------------------------+---------+------------+----------------------------+
| Parameter / script name                    | Type    | Default    | Function                   |
+============================================+=========+============+============================+
| Number of scales / ``Number_of_scales``    | Integer | 5          |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Minimal scale (%) / ``Minimal_scale_``     | Double  | 25         |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Allow angle / ``Allow_angle``              | Choice  | Any        | |                          |
|                                            |         |            | | **0 deg.**               |
|                                            |         |            | | **180 deg.**             |
|                                            |         |            | | **90 deg.**              |
|                                            |         |            | | **Any**                  |
+--------------------------------------------+---------+------------+----------------------------+
| Spacing / ``Spacing``                      | Integer | 1          |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Precision / ``Precision``                  | Integer | 7          |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Masking / ``Masking``                      | Choice  | No masking | |                          |
|                                            |         |            | | **No masking**           |
|                                            |         |            | | **Mask as bottom layer** |
+--------------------------------------------+---------+------------+----------------------------+
| Width / ``Width``                          | Integer | 512        |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Height / ``Height``                        | Integer | 512        |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0    | |                          |
|                                            |         |            | | **Merged**               |
|                                            |         |            | | **Layer 0**              |
|                                            |         |            | | **Layer 1**              |
|                                            |         |            | | **Layer 2**              |
|                                            |         |            | | **Layer 3**              |
|                                            |         |            | | **Layer 4**              |
|                                            |         |            | | **Layer 5**              |
|                                            |         |            | | **Layer 6**              |
|                                            |         |            | | **Layer 7**              |
|                                            |         |            | | **Layer 8**              |
|                                            |         |            | | **Layer 9**              |
+--------------------------------------------+---------+------------+----------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic    | |                          |
|                                            |         |            | | **Fixed (Inplace)**      |
|                                            |         |            | | **Dynamic**              |
|                                            |         |            | | **Downsample 1/2**       |
|                                            |         |            | | **Downsample 1/4**       |
|                                            |         |            | | **Downsample 1/8**       |
|                                            |         |            | | **Downsample 1/16**      |
+--------------------------------------------+---------+------------+----------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off        |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off        |                            |
+--------------------------------------------+---------+------------+----------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off        | |                          |
|                                            |         |            | | **Off**                  |
|                                            |         |            | | **Level 1**              |
|                                            |         |            | | **Level 2**              |
|                                            |         |            | | **Level 3**              |
+--------------------------------------------+---------+------------+----------------------------+
