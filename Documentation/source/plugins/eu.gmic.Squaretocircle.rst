.. _eu.gmic.Squaretocircle:

G’MIC Square to circle node
===========================

*This documentation is for version 1.0 of G’MIC Square to circle.*

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

+-----------------------------------+---------+------------------+------------------------+
| Parameter / script name           | Type    | Default          | Function               |
+===================================+=========+==================+========================+
| Mode / ``Mode``                   | Choice  | Square to circle | |                      |
|                                   |         |                  | | **Square to circle** |
|                                   |         |                  | | **Circle to square** |
+-----------------------------------+---------+------------------+------------------------+
| Interpolation / ``Interpolation`` | Choice  | Linear           | |                      |
|                                   |         |                  | | **Nearest neighbor** |
|                                   |         |                  | | **Linear**           |
+-----------------------------------+---------+------------------+------------------------+
| Boundary / ``Boundary``           | Choice  | Transparent      | |                      |
|                                   |         |                  | | **Transparent**      |
|                                   |         |                  | | **Nearest**          |
|                                   |         |                  | | **Periodic**         |
|                                   |         |                  | | **Mirror**           |
+-----------------------------------+---------+------------------+------------------------+
| X-factor (%) / ``Xfactor_``       | Double  | 0                |                        |
+-----------------------------------+---------+------------------+------------------------+
| Y-factor (%) / ``Yfactor_``       | Double  | 0                |                        |
+-----------------------------------+---------+------------------+------------------------+
| X-offset (%) / ``Xoffset_``       | Double  | 0                |                        |
+-----------------------------------+---------+------------------+------------------------+
| Y-offset (%) / ``Yoffset_``       | Double  | 0                |                        |
+-----------------------------------+---------+------------------+------------------------+
| Output Layer / ``Output_Layer``   | Choice  | Layer 0          | |                      |
|                                   |         |                  | | **Merged**           |
|                                   |         |                  | | **Layer 0**          |
|                                   |         |                  | | **Layer -1**         |
|                                   |         |                  | | **Layer -2**         |
|                                   |         |                  | | **Layer -3**         |
|                                   |         |                  | | **Layer -4**         |
|                                   |         |                  | | **Layer -5**         |
|                                   |         |                  | | **Layer -6**         |
|                                   |         |                  | | **Layer -7**         |
|                                   |         |                  | | **Layer -8**         |
|                                   |         |                  | | **Layer -9**         |
+-----------------------------------+---------+------------------+------------------------+
| Resize Mode / ``Resize_Mode``     | Choice  | Dynamic          | |                      |
|                                   |         |                  | | **Fixed (Inplace)**  |
|                                   |         |                  | | **Dynamic**          |
|                                   |         |                  | | **Downsample 1/2**   |
|                                   |         |                  | | **Downsample 1/4**   |
|                                   |         |                  | | **Downsample 1/8**   |
|                                   |         |                  | | **Downsample 1/16**  |
+-----------------------------------+---------+------------------+------------------------+
| Ignore Alpha / ``Ignore_Alpha``   | Boolean | Off              |                        |
+-----------------------------------+---------+------------------+------------------------+
| Log Verbosity / ``Log_Verbosity`` | Choice  | Off              | |                      |
|                                   |         |                  | | **Off**              |
|                                   |         |                  | | **Level 1**          |
|                                   |         |                  | | **Level 2**          |
|                                   |         |                  | | **Level 3**          |
+-----------------------------------+---------+------------------+------------------------+
