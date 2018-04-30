.. _eu.gmic.Dices:

G’MIC Dices node
================

*This documentation is for version 0.3 of G’MIC Dices.*

Description
-----------

Author: David Tschumperle. Latest update: 2013/27/06.

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

+-----------------------------------+---------+-------------+----------------------------------+
| Parameter / script name           | Type    | Default     | Function                         |
+===================================+=========+=============+==================================+
| Resolution / ``Resolution``       | Double  | 2           |                                  |
+-----------------------------------+---------+-------------+----------------------------------+
| Size / ``Size``                   | Integer | 24          |                                  |
+-----------------------------------+---------+-------------+----------------------------------+
| Color model / ``Color_model``     | Choice  | White dices | |                                |
|                                   |         |             | | **Black dices**                |
|                                   |         |             | | **White dices**                |
|                                   |         |             | | **Dices with colored numbers** |
|                                   |         |             | | **Dices with colored sides**   |
+-----------------------------------+---------+-------------+----------------------------------+
| Output Layer / ``Output_Layer``   | Choice  | Layer 0     | |                                |
|                                   |         |             | | **Merged**                     |
|                                   |         |             | | **Layer 0**                    |
|                                   |         |             | | **Layer 1**                    |
|                                   |         |             | | **Layer 2**                    |
|                                   |         |             | | **Layer 3**                    |
|                                   |         |             | | **Layer 4**                    |
|                                   |         |             | | **Layer 5**                    |
|                                   |         |             | | **Layer 6**                    |
|                                   |         |             | | **Layer 7**                    |
|                                   |         |             | | **Layer 8**                    |
|                                   |         |             | | **Layer 9**                    |
+-----------------------------------+---------+-------------+----------------------------------+
| Resize Mode / ``Resize_Mode``     | Choice  | Dynamic     | |                                |
|                                   |         |             | | **Fixed (Inplace)**            |
|                                   |         |             | | **Dynamic**                    |
|                                   |         |             | | **Downsample 1/2**             |
|                                   |         |             | | **Downsample 1/4**             |
|                                   |         |             | | **Downsample 1/8**             |
|                                   |         |             | | **Downsample 1/16**            |
+-----------------------------------+---------+-------------+----------------------------------+
| Ignore Alpha / ``Ignore_Alpha``   | Boolean | Off         |                                  |
+-----------------------------------+---------+-------------+----------------------------------+
| Log Verbosity / ``Log_Verbosity`` | Choice  | Off         | |                                |
|                                   |         |             | | **Off**                        |
|                                   |         |             | | **Level 1**                    |
|                                   |         |             | | **Level 2**                    |
|                                   |         |             | | **Level 3**                    |
+-----------------------------------+---------+-------------+----------------------------------+
