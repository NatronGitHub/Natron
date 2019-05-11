.. _eu.gmic.Spatialtransition:

G’MIC Spatial transition node
=============================

*This documentation is for version 1.0 of G’MIC Spatial transition.*

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

+-----------------------------------------------------+---------+--------------------+-----------------------+
| Parameter / script name                             | Type    | Default            | Function              |
+=====================================================+=========+====================+=======================+
| Number of added frames / ``Number_of_added_frames`` | Integer | 10                 |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Shading (%) / ``Shading_``                          | Double  | 0                  |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Transition shape / ``Transition_shape``             | Choice  | Plasma             | |                     |
|                                                     |         |                    | | **Bottom layer**    |
|                                                     |         |                    | | **Top layer**       |
|                                                     |         |                    | | **Custom formula**  |
|                                                     |         |                    | | **Horizontal**      |
|                                                     |         |                    | | **Vertical**        |
|                                                     |         |                    | | **Angular**         |
|                                                     |         |                    | | **Radial**          |
|                                                     |         |                    | | **Plasma**          |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Custom formula / ``Custom_formula``                 | String  | cos(x*y/(16+32*A)) |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| A-value / ``Avalue``                                | Double  | 0                  |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Preview type / ``Preview_type``                     | Choice  | Timed image        | |                     |
|                                                     |         |                    | | **Transition map**  |
|                                                     |         |                    | | **Timed image**     |
|                                                     |         |                    | | **Sequence x4**     |
|                                                     |         |                    | | **Sequence x6**     |
|                                                     |         |                    | | **Sequence x8**     |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Preview time / ``Preview_time``                     | Double  | 0.5                |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Output Layer / ``Output_Layer``                     | Choice  | Layer 0            | |                     |
|                                                     |         |                    | | **Merged**          |
|                                                     |         |                    | | **Layer 0**         |
|                                                     |         |                    | | **Layer -1**        |
|                                                     |         |                    | | **Layer -2**        |
|                                                     |         |                    | | **Layer -3**        |
|                                                     |         |                    | | **Layer -4**        |
|                                                     |         |                    | | **Layer -5**        |
|                                                     |         |                    | | **Layer -6**        |
|                                                     |         |                    | | **Layer -7**        |
|                                                     |         |                    | | **Layer -8**        |
|                                                     |         |                    | | **Layer -9**        |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Resize Mode / ``Resize_Mode``                       | Choice  | Dynamic            | |                     |
|                                                     |         |                    | | **Fixed (Inplace)** |
|                                                     |         |                    | | **Dynamic**         |
|                                                     |         |                    | | **Downsample 1/2**  |
|                                                     |         |                    | | **Downsample 1/4**  |
|                                                     |         |                    | | **Downsample 1/8**  |
|                                                     |         |                    | | **Downsample 1/16** |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``                     | Boolean | Off                |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``          | Boolean | Off                |                       |
+-----------------------------------------------------+---------+--------------------+-----------------------+
| Log Verbosity / ``Log_Verbosity``                   | Choice  | Off                | |                     |
|                                                     |         |                    | | **Off**             |
|                                                     |         |                    | | **Level 1**         |
|                                                     |         |                    | | **Level 2**         |
|                                                     |         |                    | | **Level 3**         |
+-----------------------------------------------------+---------+--------------------+-----------------------+
