.. _eu.gmic.Tonestolayers:

G’MIC Tones to layers node
==========================

*This documentation is for version 1.0 of G’MIC Tones to layers.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

+----------+-------------+----------+
| Input    | Description | Optional |
+==========+=============+==========+
| Source   |             | No       |
+----------+-------------+----------+
| Layer -1 |             | Yes      |
+----------+-------------+----------+
| Layer -2 |             | Yes      |
+----------+-------------+----------+
| Layer -3 |             | Yes      |
+----------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                    | Type    | Default | Function              |
+============================================+=========+=========+=======================+
| Number of tones / ``Number_of_tones``      | Integer | 3       |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Start of mid-tones / ``Start_of_midtones`` | Integer | 85      |                       |
+--------------------------------------------+---------+---------+-----------------------+
| End of mid-tones / ``End_of_midtones``     | Integer | 170     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Smoothness / ``Smoothness``                | Double  | 0.5     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Alpha / ``Alpha``                          | Choice  | Binary  | |                     |
|                                            |         |         | | **Binary**          |
|                                            |         |         | | **Scalar**          |
+--------------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                     |
|                                            |         |         | | **Merged**          |
|                                            |         |         | | **Layer 0**         |
|                                            |         |         | | **Layer -1**        |
|                                            |         |         | | **Layer -2**        |
|                                            |         |         | | **Layer -3**        |
|                                            |         |         | | **Layer -4**        |
|                                            |         |         | | **Layer -5**        |
|                                            |         |         | | **Layer -6**        |
|                                            |         |         | | **Layer -7**        |
|                                            |         |         | | **Layer -8**        |
|                                            |         |         | | **Layer -9**        |
+--------------------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                     |
|                                            |         |         | | **Fixed (Inplace)** |
|                                            |         |         | | **Dynamic**         |
|                                            |         |         | | **Downsample 1/2**  |
|                                            |         |         | | **Downsample 1/4**  |
|                                            |         |         | | **Downsample 1/8**  |
|                                            |         |         | | **Downsample 1/16** |
+--------------------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                     |
|                                            |         |         | | **Off**             |
|                                            |         |         | | **Level 1**         |
|                                            |         |         | | **Level 2**         |
|                                            |         |         | | **Level 3**         |
+--------------------------------------------+---------+---------+-----------------------+
