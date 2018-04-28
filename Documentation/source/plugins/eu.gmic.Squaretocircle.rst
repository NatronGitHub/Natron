.. _eu.gmic.Squaretocircle:

G’MIC Square to circle node
===========================

*This documentation is for version 0.3 of G’MIC Square to circle.*

Description
-----------

This filter implements the mapping functions described in this page, by C. Fong:

http://squircular.blogspot.com/2015/09/mapping-circle-to-square.html

Author: David Tschumperle. Latest update: 2017/10/30.

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

+--------------------------------------------+---------+------------------+---------------------------+
| Parameter / script name                    | Type    | Default          | Function                  |
+============================================+=========+==================+===========================+
| Mode / ``Mode``                            | Choice  | Square to circle | |                         |
|                                            |         |                  | | **Square to circle**    |
|                                            |         |                  | | **Circle to square**    |
+--------------------------------------------+---------+------------------+---------------------------+
| Interpolation / ``Interpolation``          | Choice  | Linear           | |                         |
|                                            |         |                  | | **Nearest neighbor**    |
|                                            |         |                  | | **Linear**              |
+--------------------------------------------+---------+------------------+---------------------------+
| Boundary / ``Boundary``                    | Choice  | Transparent      | |                         |
|                                            |         |                  | | **Transparent**         |
|                                            |         |                  | | **Nearest**             |
|                                            |         |                  | | **Periodic**            |
|                                            |         |                  | | **Mirror**              |
+--------------------------------------------+---------+------------------+---------------------------+
| X-factor (%) / ``Xfactor_``                | Double  | 0                |                           |
+--------------------------------------------+---------+------------------+---------------------------+
| Y-factor (%) / ``Yfactor_``                | Double  | 0                |                           |
+--------------------------------------------+---------+------------------+---------------------------+
| X-offset (%) / ``Xoffset_``                | Double  | 0                |                           |
+--------------------------------------------+---------+------------------+---------------------------+
| Y-offset (%) / ``Yoffset_``                | Double  | 0                |                           |
+--------------------------------------------+---------+------------------+---------------------------+
| Preview type / ``Preview_type``            | Choice  | Full             | |                         |
|                                            |         |                  | | **Full**                |
|                                            |         |                  | | **Forward horizontal**  |
|                                            |         |                  | | **Forward vertical**    |
|                                            |         |                  | | **Backward horizontal** |
|                                            |         |                  | | **Backward vertical**   |
|                                            |         |                  | | **Duplicate top**       |
|                                            |         |                  | | **Duplicate left**      |
|                                            |         |                  | | **Duplicate bottom**    |
|                                            |         |                  | | **Duplicate right**     |
+--------------------------------------------+---------+------------------+---------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0          | |                         |
|                                            |         |                  | | **Merged**              |
|                                            |         |                  | | **Layer 0**             |
|                                            |         |                  | | **Layer 1**             |
|                                            |         |                  | | **Layer 2**             |
|                                            |         |                  | | **Layer 3**             |
|                                            |         |                  | | **Layer 4**             |
|                                            |         |                  | | **Layer 5**             |
|                                            |         |                  | | **Layer 6**             |
|                                            |         |                  | | **Layer 7**             |
|                                            |         |                  | | **Layer 8**             |
|                                            |         |                  | | **Layer 9**             |
+--------------------------------------------+---------+------------------+---------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic          | |                         |
|                                            |         |                  | | **Fixed (Inplace)**     |
|                                            |         |                  | | **Dynamic**             |
|                                            |         |                  | | **Downsample 1/2**      |
|                                            |         |                  | | **Downsample 1/4**      |
|                                            |         |                  | | **Downsample 1/8**      |
|                                            |         |                  | | **Downsample 1/16**     |
+--------------------------------------------+---------+------------------+---------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off              |                           |
+--------------------------------------------+---------+------------------+---------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off              |                           |
+--------------------------------------------+---------+------------------+---------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off              | |                         |
|                                            |         |                  | | **Off**                 |
|                                            |         |                  | | **Level 1**             |
|                                            |         |                  | | **Level 2**             |
|                                            |         |                  | | **Level 3**             |
+--------------------------------------------+---------+------------------+---------------------------+
