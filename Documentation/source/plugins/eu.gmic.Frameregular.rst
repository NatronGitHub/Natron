.. _eu.gmic.Frameregular:

G’MIC Frame regular node
========================

*This documentation is for version 0.3 of G’MIC Frame regular.*

Description
-----------

Crop parameters :

Frame parameters :

Author: David Tschumperle. Latest update: 2010/29/12.

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

+-----------------------------------+---------+----------------+-----------------------+
| Parameter / script name           | Type    | Default        | Function              |
+===================================+=========+================+=======================+
| X-start (%) / ``Xstart_``         | Integer | 0              |                       |
+-----------------------------------+---------+----------------+-----------------------+
| X-end (%) / ``Xend_``             | Integer | 100            |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Y-start (%) / ``Ystart_``         | Integer | 0              |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Y-end (%) / ``Yend_``             | Integer | 100            |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Width (%) / ``Width_``            | Integer | 10             |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Height (%) / ``Height_``          | Integer | 10             |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Color / ``Color``                 | Color   | r: 0 g: 0 b: 0 |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Outline size / ``Outline_size``   | Integer | 1              |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Outline color / ``Outline_color`` | Color   | r: 1 g: 1 b: 1 |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Output Layer / ``Output_Layer``   | Choice  | Layer 0        | |                     |
|                                   |         |                | | **Merged**          |
|                                   |         |                | | **Layer 0**         |
|                                   |         |                | | **Layer 1**         |
|                                   |         |                | | **Layer 2**         |
|                                   |         |                | | **Layer 3**         |
|                                   |         |                | | **Layer 4**         |
|                                   |         |                | | **Layer 5**         |
|                                   |         |                | | **Layer 6**         |
|                                   |         |                | | **Layer 7**         |
|                                   |         |                | | **Layer 8**         |
|                                   |         |                | | **Layer 9**         |
+-----------------------------------+---------+----------------+-----------------------+
| Resize Mode / ``Resize_Mode``     | Choice  | Dynamic        | |                     |
|                                   |         |                | | **Fixed (Inplace)** |
|                                   |         |                | | **Dynamic**         |
|                                   |         |                | | **Downsample 1/2**  |
|                                   |         |                | | **Downsample 1/4**  |
|                                   |         |                | | **Downsample 1/8**  |
|                                   |         |                | | **Downsample 1/16** |
+-----------------------------------+---------+----------------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``   | Boolean | Off            |                       |
+-----------------------------------+---------+----------------+-----------------------+
| Log Verbosity / ``Log_Verbosity`` | Choice  | Off            | |                     |
|                                   |         |                | | **Off**             |
|                                   |         |                | | **Level 1**         |
|                                   |         |                | | **Level 2**         |
|                                   |         |                | | **Level 3**         |
+-----------------------------------+---------+----------------+-----------------------+
