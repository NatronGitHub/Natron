.. _eu.gmic.Chessboard:

G’MIC Chessboard node
=====================

*This documentation is for version 1.0 of G’MIC Chessboard.*

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

+--------------------------------------------+---------+---------------------+----------------------------+
| Parameter / script name                    | Type    | Default             | Function                   |
+============================================+=========+=====================+============================+
| First size / ``First_size``                | Integer | 64                  |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Second size / ``Second_size``              | Integer | 64                  |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| First offset / ``First_offset``            | Integer | 0                   |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Second offset / ``Second_offset``          | Integer | 0                   |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Angle / ``Angle``                          | Double  | 0                   |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Opacity / ``Opacity``                      | Double  | 0.5                 |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| First color / ``First_color``              | Color   | r: 0 g: 0 b: 0 a: 0 |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Second color / ``Second_color``            | Color   | r: 1 g: 1 b: 1 a: 1 |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Preview type / ``Preview_type``            | Choice  | Full                | |                          |
|                                            |         |                     | | **Full**                 |
|                                            |         |                     | | **Forward horizontal**   |
|                                            |         |                     | | **Forward vertical**     |
|                                            |         |                     | | **Backward horizontal**  |
|                                            |         |                     | | **Backward vertical**    |
|                                            |         |                     | | **Duplicate top**        |
|                                            |         |                     | | **Duplicate left**       |
|                                            |         |                     | | **Duplicate bottom**     |
|                                            |         |                     | | **Duplicate right**      |
|                                            |         |                     | | **Duplicate horizontal** |
|                                            |         |                     | | **Duplicate vertical**   |
|                                            |         |                     | | **Checkered**            |
|                                            |         |                     | | **Checkered inverse**    |
+--------------------------------------------+---------+---------------------+----------------------------+
| Preview split / ``Preview_split``          | Double  | x: 0.5 y: 0.5       |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0             | |                          |
|                                            |         |                     | | **Merged**               |
|                                            |         |                     | | **Layer 0**              |
|                                            |         |                     | | **Layer -1**             |
|                                            |         |                     | | **Layer -2**             |
|                                            |         |                     | | **Layer -3**             |
|                                            |         |                     | | **Layer -4**             |
|                                            |         |                     | | **Layer -5**             |
|                                            |         |                     | | **Layer -6**             |
|                                            |         |                     | | **Layer -7**             |
|                                            |         |                     | | **Layer -8**             |
|                                            |         |                     | | **Layer -9**             |
+--------------------------------------------+---------+---------------------+----------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic             | |                          |
|                                            |         |                     | | **Fixed (Inplace)**      |
|                                            |         |                     | | **Dynamic**              |
|                                            |         |                     | | **Downsample 1/2**       |
|                                            |         |                     | | **Downsample 1/4**       |
|                                            |         |                     | | **Downsample 1/8**       |
|                                            |         |                     | | **Downsample 1/16**      |
+--------------------------------------------+---------+---------------------+----------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off                 |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off                 |                            |
+--------------------------------------------+---------+---------------------+----------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off                 | |                          |
|                                            |         |                     | | **Off**                  |
|                                            |         |                     | | **Level 1**              |
|                                            |         |                     | | **Level 2**              |
|                                            |         |                     | | **Level 3**              |
+--------------------------------------------+---------+---------------------+----------------------------+
