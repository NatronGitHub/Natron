.. _eu.gmic.MakeSeamlessPatchBased:

G’MIC Make Seamless Patch-Based node
====================================

*This documentation is for version 1.0 of G’MIC Make Seamless Patch-Based.*

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

+--------------------------------------------+---------+---------------+----------------------------+
| Parameter / script name                    | Type    | Default       | Function                   |
+============================================+=========+===============+============================+
| Frame Size / ``Frame_Size``                | Integer | 32            |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Patch Size / ``Patch_Size``                | Integer | 9             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Blend Size / ``Blend_Size``                | Integer | 0             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Frame Type / ``Frame_Type``                | Choice  | Outer         | |                          |
|                                            |         |               | | **Inner**                |
|                                            |         |               | | **Outer**                |
+--------------------------------------------+---------+---------------+----------------------------+
| Equalize Light / ``Equalize_Light``        | Double  | 100           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview Original / ``Preview_Original``    | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Tiled Preview / ``Tiled_Preview``          | Choice  | 2x2           | |                          |
|                                            |         |               | | **None**                 |
|                                            |         |               | | **2x1**                  |
|                                            |         |               | | **1x2**                  |
|                                            |         |               | | **2x2**                  |
|                                            |         |               | | **3x3**                  |
|                                            |         |               | | **4x4**                  |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview Type / ``Preview_Type``            | Choice  | Full          | |                          |
|                                            |         |               | | **Full**                 |
|                                            |         |               | | **Forward Horizontal**   |
|                                            |         |               | | **Forward Vertical**     |
|                                            |         |               | | **Backward Horizontal**  |
|                                            |         |               | | **Backward Vertical**    |
|                                            |         |               | | **Duplicate Top**        |
|                                            |         |               | | **Duplicate Left**       |
|                                            |         |               | | **Duplicate Bottom**     |
|                                            |         |               | | **Duplicate Right**      |
|                                            |         |               | | **Duplicate Horizontal** |
|                                            |         |               | | **Duplicate Vertical**   |
|                                            |         |               | | **Checkered**            |
|                                            |         |               | | **Checkered Inverse**    |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview Split / ``Preview_Split``          | Double  | x: 0.5 y: 0.5 |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0       | |                          |
|                                            |         |               | | **Merged**               |
|                                            |         |               | | **Layer 0**              |
|                                            |         |               | | **Layer -1**             |
|                                            |         |               | | **Layer -2**             |
|                                            |         |               | | **Layer -3**             |
|                                            |         |               | | **Layer -4**             |
|                                            |         |               | | **Layer -5**             |
|                                            |         |               | | **Layer -6**             |
|                                            |         |               | | **Layer -7**             |
|                                            |         |               | | **Layer -8**             |
|                                            |         |               | | **Layer -9**             |
+--------------------------------------------+---------+---------------+----------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic       | |                          |
|                                            |         |               | | **Fixed (Inplace)**      |
|                                            |         |               | | **Dynamic**              |
|                                            |         |               | | **Downsample 1/2**       |
|                                            |         |               | | **Downsample 1/4**       |
|                                            |         |               | | **Downsample 1/8**       |
|                                            |         |               | | **Downsample 1/16**      |
+--------------------------------------------+---------+---------------+----------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off           | |                          |
|                                            |         |               | | **Off**                  |
|                                            |         |               | | **Level 1**              |
|                                            |         |               | | **Level 2**              |
|                                            |         |               | | **Level 3**              |
+--------------------------------------------+---------+---------------+----------------------------+
