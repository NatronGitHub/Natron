.. _eu.gmic.Pixelsort:

G’MIC Pixel sort node
=====================

*This documentation is for version 1.0 of G’MIC Pixel sort.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

+--------+-------------+----------+
| Input  | Description | Optional |
+========+=============+==========+
| Source |             | No       |
+--------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------------------------------+---------+------------+--------------------------+
| Parameter / script name                                | Type    | Default    | Function                 |
+========================================================+=========+============+==========================+
| Order / ``Order``                                      | Choice  | Increasing | |                        |
|                                                        |         |            | | **Decreasing**         |
|                                                        |         |            | | **Increasing**         |
+--------------------------------------------------------+---------+------------+--------------------------+
| Axis / ``Axis``                                        | Choice  | X-axis     | |                        |
|                                                        |         |            | | **X-axis**             |
|                                                        |         |            | | **Y-axis**             |
|                                                        |         |            | | **X-axis then Y-axis** |
|                                                        |         |            | | **Y-axis then X-axis** |
+--------------------------------------------------------+---------+------------+--------------------------+
| Sorting criterion / ``Sorting_criterion``              | Choice  | Red        | |                        |
|                                                        |         |            | | **Red**                |
|                                                        |         |            | | **Green**              |
|                                                        |         |            | | **Blue**               |
|                                                        |         |            | | **Intensity**          |
|                                                        |         |            | | **Luminance**          |
|                                                        |         |            | | **Lightness**          |
|                                                        |         |            | | **Hue**                |
|                                                        |         |            | | **Saturation**         |
|                                                        |         |            | | **Minimum**            |
|                                                        |         |            | | **Maximum**            |
|                                                        |         |            | | **Random**             |
+--------------------------------------------------------+---------+------------+--------------------------+
| Mask by / ``Mask_by``                                  | Choice  | Criterion  | |                        |
|                                                        |         |            | | **Bottom layer**       |
|                                                        |         |            | | **Criterion**          |
|                                                        |         |            | | **Contours**           |
|                                                        |         |            | | **Random**             |
+--------------------------------------------------------+---------+------------+--------------------------+
| Lower mask threshold (%) / ``Lower_mask_threshold_``   | Double  | 0          |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Higher mask threshold (%) / ``Higher_mask_threshold_`` | Double  | 100        |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Mask smoothness (%) / ``Mask_smoothness_``             | Double  | 0          |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Invert mask / ``Invert_mask``                          | Boolean | Off        |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Preview mask / ``Preview_mask``                        | Boolean | Off        |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Output Layer / ``Output_Layer``                        | Choice  | Layer 0    | |                        |
|                                                        |         |            | | **Merged**             |
|                                                        |         |            | | **Layer 0**            |
|                                                        |         |            | | **Layer -1**           |
|                                                        |         |            | | **Layer -2**           |
|                                                        |         |            | | **Layer -3**           |
|                                                        |         |            | | **Layer -4**           |
|                                                        |         |            | | **Layer -5**           |
|                                                        |         |            | | **Layer -6**           |
|                                                        |         |            | | **Layer -7**           |
|                                                        |         |            | | **Layer -8**           |
|                                                        |         |            | | **Layer -9**           |
+--------------------------------------------------------+---------+------------+--------------------------+
| Resize Mode / ``Resize_Mode``                          | Choice  | Dynamic    | |                        |
|                                                        |         |            | | **Fixed (Inplace)**    |
|                                                        |         |            | | **Dynamic**            |
|                                                        |         |            | | **Downsample 1/2**     |
|                                                        |         |            | | **Downsample 1/4**     |
|                                                        |         |            | | **Downsample 1/8**     |
|                                                        |         |            | | **Downsample 1/16**    |
+--------------------------------------------------------+---------+------------+--------------------------+
| Ignore Alpha / ``Ignore_Alpha``                        | Boolean | Off        |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``             | Boolean | Off        |                          |
+--------------------------------------------------------+---------+------------+--------------------------+
| Log Verbosity / ``Log_Verbosity``                      | Choice  | Off        | |                        |
|                                                        |         |            | | **Off**                |
|                                                        |         |            | | **Level 1**            |
|                                                        |         |            | | **Level 2**            |
|                                                        |         |            | | **Level 3**            |
+--------------------------------------------------------+---------+------------+--------------------------+
