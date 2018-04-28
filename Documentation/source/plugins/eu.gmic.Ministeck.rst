.. _eu.gmic.Ministeck:

G’MIC Ministeck node
====================

*This documentation is for version 0.3 of G’MIC Ministeck.*

Description
-----------

Author: David Tschumperle. Latest update: 2015/14/01.

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

+--------------------------------------------+---------+---------+-----------------------+
| Parameter / script name                    | Type    | Default | Function              |
+============================================+=========+=========+=======================+
| Number of colors / ``Number_of_colors``    | Integer | 8       |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Resolution (px) / ``Resolution_px``        | Integer | 64      |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Piece size (px) / ``Piece_size_px``        | Integer | 8       |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Piece complexity / ``Piece_complexity``    | Integer | 2       |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Relief amplitude / ``Relief_amplitude``    | Double  | 100     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Relief size / ``Relief_size``              | Double  | 0.3     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Add 1px outline / ``Add_1px_outline``      | Boolean | Off     |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                     |
|                                            |         |         | | **Merged**          |
|                                            |         |         | | **Layer 0**         |
|                                            |         |         | | **Layer 1**         |
|                                            |         |         | | **Layer 2**         |
|                                            |         |         | | **Layer 3**         |
|                                            |         |         | | **Layer 4**         |
|                                            |         |         | | **Layer 5**         |
|                                            |         |         | | **Layer 6**         |
|                                            |         |         | | **Layer 7**         |
|                                            |         |         | | **Layer 8**         |
|                                            |         |         | | **Layer 9**         |
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
