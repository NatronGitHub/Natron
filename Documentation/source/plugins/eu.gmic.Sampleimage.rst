.. _eu.gmic.Sampleimage:

G’MIC Sample image node
=======================

*This documentation is for version 0.3 of G’MIC Sample image.*

Description
-----------

Choosing 0 for parameters Width or Height means Automatic.

Author: David Tschumperle. Latest update: 2017/16/01.

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
| Input / ``Input``                          | Choice  | Random  | |                     |
|                                            |         |         | | **Random**          |
|                                            |         |         | | **Apples**          |
|                                            |         |         | | **Barbara**         |
|                                            |         |         | | **Boats**           |
|                                            |         |         | | **Bottles**         |
|                                            |         |         | | **Butterfly**       |
|                                            |         |         | | **Cameraman**       |
|                                            |         |         | | **Car**             |
|                                            |         |         | | **Cat**             |
|                                            |         |         | | **Cliff**           |
|                                            |         |         | | **David**           |
|                                            |         |         | | **Dog**             |
|                                            |         |         | | **Duck**            |
|                                            |         |         | | **Eagle**           |
|                                            |         |         | | **Elephant**        |
|                                            |         |         | | **Earth**           |
|                                            |         |         | | **Flower**          |
|                                            |         |         | | **Fruits**          |
|                                            |         |         | | **Greece**          |
|                                            |         |         | | **Gummy**           |
|                                            |         |         | | **House**           |
|                                            |         |         | | **Inside**          |
|                                            |         |         | | **Landscape**       |
|                                            |         |         | | **Leaf**            |
|                                            |         |         | | **Lena**            |
|                                            |         |         | | **Leno**            |
|                                            |         |         | | **Lion**            |
|                                            |         |         | | **Mandrill**        |
|                                            |         |         | | **Mona Lisa**       |
|                                            |         |         | | **Monkey**          |
|                                            |         |         | | **Parrots**         |
|                                            |         |         | | **Pencils**         |
|                                            |         |         | | **Peppers**         |
|                                            |         |         | | **Rooster**         |
|                                            |         |         | | **Rose**            |
|                                            |         |         | | **Square**          |
|                                            |         |         | | **Teddy**           |
|                                            |         |         | | **Tiger**           |
|                                            |         |         | | **Wall**            |
|                                            |         |         | | **Waterfall**       |
|                                            |         |         | | **Zelda**           |
+--------------------------------------------+---------+---------+-----------------------+
| Width / ``Width``                          | Integer | 0       |                       |
+--------------------------------------------+---------+---------+-----------------------+
| Height / ``Height``                        | Integer | 0       |                       |
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
