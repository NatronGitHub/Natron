.. _eu.gmic.Pack:

G’MIC Pack node
===============

*This documentation is for version 0.3 of G’MIC Pack.*

Description
-----------

This filter tries to pack all input layers into a single image, while trying to minimize the empty areas.

This problem being NP-hard, the algorithm finds (of course) a non-optimal, but often acceptable solution to this packing problem.

Author: David Tschumperle. Latest update: 2015/11/05.

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com).

Inputs
------

+-----------+-------------+----------+
| Input     | Description | Optional |
+===========+=============+==========+
| Input     |             | No       |
+-----------+-------------+----------+
| Ext. In 1 |             | Yes      |
+-----------+-------------+----------+
| Ext. In 2 |             | Yes      |
+-----------+-------------+----------+
| Ext. In 3 |             | Yes      |
+-----------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+-------------------------------------------------------+---------+-------------------+-------------------------+
| Parameter / script name                               | Type    | Default           | Function                |
+=======================================================+=========+===================+=========================+
| Order by / ``Order_by``                               | Choice  | Maximum dimension | |                       |
|                                                       |         |                   | | **Width**             |
|                                                       |         |                   | | **Height**            |
|                                                       |         |                   | | **Maximum dimension** |
|                                                       |         |                   | | **Area**              |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Tends to be square / ``Tends_to_be_square``           | Boolean | Off               |                         |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Force transparency / ``Force_transparency``           | Boolean | On                |                         |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Output coordinates file / ``Output_coordinates_file`` | Boolean | Off               |                         |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Output folder / ``Output_folder``                     | N/A     |                   |                         |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Output Layer / ``Output_Layer``                       | Choice  | Layer 0           | |                       |
|                                                       |         |                   | | **Merged**            |
|                                                       |         |                   | | **Layer 0**           |
|                                                       |         |                   | | **Layer 1**           |
|                                                       |         |                   | | **Layer 2**           |
|                                                       |         |                   | | **Layer 3**           |
|                                                       |         |                   | | **Layer 4**           |
|                                                       |         |                   | | **Layer 5**           |
|                                                       |         |                   | | **Layer 6**           |
|                                                       |         |                   | | **Layer 7**           |
|                                                       |         |                   | | **Layer 8**           |
|                                                       |         |                   | | **Layer 9**           |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Resize Mode / ``Resize_Mode``                         | Choice  | Dynamic           | |                       |
|                                                       |         |                   | | **Fixed (Inplace)**   |
|                                                       |         |                   | | **Dynamic**           |
|                                                       |         |                   | | **Downsample 1/2**    |
|                                                       |         |                   | | **Downsample 1/4**    |
|                                                       |         |                   | | **Downsample 1/8**    |
|                                                       |         |                   | | **Downsample 1/16**   |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Ignore Alpha / ``Ignore_Alpha``                       | Boolean | Off               |                         |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``            | Boolean | Off               |                         |
+-------------------------------------------------------+---------+-------------------+-------------------------+
| Log Verbosity / ``Log_Verbosity``                     | Choice  | Off               | |                       |
|                                                       |         |                   | | **Off**               |
|                                                       |         |                   | | **Level 1**           |
|                                                       |         |                   | | **Level 2**           |
|                                                       |         |                   | | **Level 3**           |
+-------------------------------------------------------+---------+-------------------+-------------------------+
