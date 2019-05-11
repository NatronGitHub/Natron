.. _eu.gmic.Quadrangle:

G’MIC Quadrangle node
=====================

*This documentation is for version 1.0 of G’MIC Quadrangle.*

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

+---------------------------------------------------+---------+-----------------+------------------------+
| Parameter / script name                           | Type    | Default         | Function               |
+===================================================+=========+=================+========================+
| Top-left vertex (%) / ``Topleft_vertex_``         | Double  | x: 0.05 y: 0.05 |                        |
+---------------------------------------------------+---------+-----------------+------------------------+
| Top-right vertex (%) / ``Topright_vertex_``       | Double  | x: 0.95 y: 0.25 |                        |
+---------------------------------------------------+---------+-----------------+------------------------+
| Bottom-right vertex (%) / ``Bottomright_vertex_`` | Double  | x: 0.6 y: 0.95  |                        |
+---------------------------------------------------+---------+-----------------+------------------------+
| Bottom-left vertex (%) / ``Bottomleft_vertex_``   | Double  | x: 0.4 y: 0.95  |                        |
+---------------------------------------------------+---------+-----------------+------------------------+
| Interpolation / ``Interpolation``                 | Choice  | Linear          | |                      |
|                                                   |         |                 | | **Nearest neighbor** |
|                                                   |         |                 | | **Linear**           |
+---------------------------------------------------+---------+-----------------+------------------------+
| Boundary / ``Boundary``                           | Choice  | Mirror          | |                      |
|                                                   |         |                 | | **Transparent**      |
|                                                   |         |                 | | **Nearest**          |
|                                                   |         |                 | | **Periodic**         |
|                                                   |         |                 | | **Mirror**           |
+---------------------------------------------------+---------+-----------------+------------------------+
| Preview type / ``Preview_type``                   | Choice  | Output          | |                      |
|                                                   |         |                 | | **Input**            |
|                                                   |         |                 | | **Output**           |
|                                                   |         |                 | | **Both**             |
+---------------------------------------------------+---------+-----------------+------------------------+
| Output Layer / ``Output_Layer``                   | Choice  | Layer 0         | |                      |
|                                                   |         |                 | | **Merged**           |
|                                                   |         |                 | | **Layer 0**          |
|                                                   |         |                 | | **Layer -1**         |
|                                                   |         |                 | | **Layer -2**         |
|                                                   |         |                 | | **Layer -3**         |
|                                                   |         |                 | | **Layer -4**         |
|                                                   |         |                 | | **Layer -5**         |
|                                                   |         |                 | | **Layer -6**         |
|                                                   |         |                 | | **Layer -7**         |
|                                                   |         |                 | | **Layer -8**         |
|                                                   |         |                 | | **Layer -9**         |
+---------------------------------------------------+---------+-----------------+------------------------+
| Resize Mode / ``Resize_Mode``                     | Choice  | Dynamic         | |                      |
|                                                   |         |                 | | **Fixed (Inplace)**  |
|                                                   |         |                 | | **Dynamic**          |
|                                                   |         |                 | | **Downsample 1/2**   |
|                                                   |         |                 | | **Downsample 1/4**   |
|                                                   |         |                 | | **Downsample 1/8**   |
|                                                   |         |                 | | **Downsample 1/16**  |
+---------------------------------------------------+---------+-----------------+------------------------+
| Ignore Alpha / ``Ignore_Alpha``                   | Boolean | Off             |                        |
+---------------------------------------------------+---------+-----------------+------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``        | Boolean | Off             |                        |
+---------------------------------------------------+---------+-----------------+------------------------+
| Log Verbosity / ``Log_Verbosity``                 | Choice  | Off             | |                      |
|                                                   |         |                 | | **Off**              |
|                                                   |         |                 | | **Level 1**          |
|                                                   |         |                 | | **Level 2**          |
|                                                   |         |                 | | **Level 3**          |
+---------------------------------------------------+---------+-----------------+------------------------+
