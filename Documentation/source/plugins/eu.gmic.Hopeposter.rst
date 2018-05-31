.. _eu.gmic.Hopeposter:

G’MIC Hope poster node
======================

*This documentation is for version 0.3 of G’MIC Hope poster.*

Description
-----------

Author: David Tschumperle. Latest update: 2013/07/11.

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

+--------------------------------------------+---------+---------+---------------------------+
| Parameter / script name                    | Type    | Default | Function                  |
+============================================+=========+=========+===========================+
| Gamma / ``Gamma``                          | Double  | 0       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Smoothness / ``Smoothness``                | Double  | 3       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Preview type / ``Preview_type``            | Choice  | Full    | |                         |
|                                            |         |         | | **Full**                |
|                                            |         |         | | **Forward horizontal**  |
|                                            |         |         | | **Forward vertical**    |
|                                            |         |         | | **Backward horizontal** |
|                                            |         |         | | **Backward vertical**   |
|                                            |         |         | | **Duplicate top**       |
|                                            |         |         | | **Duplicate left**      |
|                                            |         |         | | **Duplicate bottom**    |
|                                            |         |         | | **Duplicate right**     |
+--------------------------------------------+---------+---------+---------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                         |
|                                            |         |         | | **Merged**              |
|                                            |         |         | | **Layer 0**             |
|                                            |         |         | | **Layer 1**             |
|                                            |         |         | | **Layer 2**             |
|                                            |         |         | | **Layer 3**             |
|                                            |         |         | | **Layer 4**             |
|                                            |         |         | | **Layer 5**             |
|                                            |         |         | | **Layer 6**             |
|                                            |         |         | | **Layer 7**             |
|                                            |         |         | | **Layer 8**             |
|                                            |         |         | | **Layer 9**             |
+--------------------------------------------+---------+---------+---------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                         |
|                                            |         |         | | **Fixed (Inplace)**     |
|                                            |         |         | | **Dynamic**             |
|                                            |         |         | | **Downsample 1/2**      |
|                                            |         |         | | **Downsample 1/4**      |
|                                            |         |         | | **Downsample 1/8**      |
|                                            |         |         | | **Downsample 1/16**     |
+--------------------------------------------+---------+---------+---------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                         |
|                                            |         |         | | **Off**                 |
|                                            |         |         | | **Level 1**             |
|                                            |         |         | | **Level 2**             |
|                                            |         |         | | **Level 3**             |
+--------------------------------------------+---------+---------+---------------------------+
