.. _eu.gmic.Colorizephotographs:

G’MIC Colorize photographs node
===============================

*This documentation is for version 0.3 of G’MIC Colorize photographs.*

Description
-----------

Note: This filter needs two layers to work properly. The bottom layer must be a B&W image, while the

top layer contains color patches that will be extrapolated in a smart way (edge-directed) to fill the entire image. At the end,

you get a completely recolored image.

Author: David Tschumperle. Latest update: 2013/16/01.

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

+--------------------------------------------+---------+-------------------------+-------------------------------+
| Parameter / script name                    | Type    | Default                 | Function                      |
+============================================+=========+=========================+===============================+
| Smoothness / ``Smoothness``                | Integer | 2                       |                               |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Anisotropy / ``Anisotropy``                | Double  | 0.2                     |                               |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Output mode / ``Output_mode``              | Choice  | Merge brightness/colors | |                             |
|                                            |         |                         | | **Merge brightness/colors** |
|                                            |         |                         | | **Split brightness/colors** |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0                 | |                             |
|                                            |         |                         | | **Merged**                  |
|                                            |         |                         | | **Layer 0**                 |
|                                            |         |                         | | **Layer 1**                 |
|                                            |         |                         | | **Layer 2**                 |
|                                            |         |                         | | **Layer 3**                 |
|                                            |         |                         | | **Layer 4**                 |
|                                            |         |                         | | **Layer 5**                 |
|                                            |         |                         | | **Layer 6**                 |
|                                            |         |                         | | **Layer 7**                 |
|                                            |         |                         | | **Layer 8**                 |
|                                            |         |                         | | **Layer 9**                 |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic                 | |                             |
|                                            |         |                         | | **Fixed (Inplace)**         |
|                                            |         |                         | | **Dynamic**                 |
|                                            |         |                         | | **Downsample 1/2**          |
|                                            |         |                         | | **Downsample 1/4**          |
|                                            |         |                         | | **Downsample 1/8**          |
|                                            |         |                         | | **Downsample 1/16**         |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off                     |                               |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off                     |                               |
+--------------------------------------------+---------+-------------------------+-------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off                     | |                             |
|                                            |         |                         | | **Off**                     |
|                                            |         |                         | | **Level 1**                 |
|                                            |         |                         | | **Level 2**                 |
|                                            |         |                         | | **Level 3**                 |
+--------------------------------------------+---------+-------------------------+-------------------------------+
