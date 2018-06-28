.. _eu.gmic.Transfercolorsadvanced:

G’MIC Transfer colors advanced node
===================================

*This documentation is for version 0.3 of G’MIC Transfer colors advanced.*

Description
-----------

Instructions:

- This filter transfers the colors of one layer to all the others.

- This is a highly experimental filter, it may be unstable or particularly long to render.

- Don’t forget to set the Input layers... option on the left to manage your input layers.

Author: David Tschumperle. Latest update: 2015/04/04.

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

+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Parameter / script name                                                                  | Type    | Default      | Function                       |
+==========================================================================================+=========+==============+================================+
| Regularization / ``Regularization``                                                      | Integer | 8            |                                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Preserve luminance / ``Preserve_luminance``                                              | Double  | 0.2          |                                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Precision / ``Precision``                                                                | Choice  | Normal       | |                              |
|                                                                                          |         |              | | **Low**                      |
|                                                                                          |         |              | | **Normal**                   |
|                                                                                          |         |              | | **High**                     |
|                                                                                          |         |              | | **Very high**                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Reference colors / ``Reference_colors``                                                  | Choice  | Bottom layer | |                              |
|                                                                                          |         |              | | **Bottom layer**             |
|                                                                                          |         |              | | **Top layer**                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Add user-defined constraints (interactive) / ``Add_userdefined_constraints_interactive`` | Boolean | Off          |                                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Preview reference / ``Preview_reference``                                                | Choice  | Up-left      | |                              |
|                                                                                          |         |              | | **None**                     |
|                                                                                          |         |              | | **Up-left**                  |
|                                                                                          |         |              | | **Up-right**                 |
|                                                                                          |         |              | | **Bottom-left**              |
|                                                                                          |         |              | | **Bottom-right**             |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Preview type / ``Preview_type``                                                          | Choice  | Full         | |                              |
|                                                                                          |         |              | | **Full**                     |
|                                                                                          |         |              | | **Forward horizontal**       |
|                                                                                          |         |              | | **Forward vertical**         |
|                                                                                          |         |              | | **Backward horizontal**      |
|                                                                                          |         |              | | **Backward vertical**        |
|                                                                                          |         |              | | **Duplicate top**            |
|                                                                                          |         |              | | **Duplicate left**           |
|                                                                                          |         |              | | **Duplicate bottom**         |
|                                                                                          |         |              | | **Duplicate right**          |
|                                                                                          |         |              | | **Duplicate horizontal**     |
|                                                                                          |         |              | | **Duplicate vertical**       |
|                                                                                          |         |              | | **Checkered**                |
|                                                                                          |         |              | | **Checkered inverse)**       |
|                                                                                          |         |              | | **Preview split = point(50** |
|                                                                                          |         |              | | **50**                       |
|                                                                                          |         |              | | **0**                        |
|                                                                                          |         |              | | **0**                        |
|                                                                                          |         |              | | **200**                      |
|                                                                                          |         |              | | **200**                      |
|                                                                                          |         |              | | **200**                      |
|                                                                                          |         |              | | **0**                        |
|                                                                                          |         |              | | **10**                       |
|                                                                                          |         |              | | **0**                        |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Output Layer / ``Output_Layer``                                                          | Choice  | Layer 0      | |                              |
|                                                                                          |         |              | | **Merged**                   |
|                                                                                          |         |              | | **Layer 0**                  |
|                                                                                          |         |              | | **Layer 1**                  |
|                                                                                          |         |              | | **Layer 2**                  |
|                                                                                          |         |              | | **Layer 3**                  |
|                                                                                          |         |              | | **Layer 4**                  |
|                                                                                          |         |              | | **Layer 5**                  |
|                                                                                          |         |              | | **Layer 6**                  |
|                                                                                          |         |              | | **Layer 7**                  |
|                                                                                          |         |              | | **Layer 8**                  |
|                                                                                          |         |              | | **Layer 9**                  |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Resize Mode / ``Resize_Mode``                                                            | Choice  | Dynamic      | |                              |
|                                                                                          |         |              | | **Fixed (Inplace)**          |
|                                                                                          |         |              | | **Dynamic**                  |
|                                                                                          |         |              | | **Downsample 1/2**           |
|                                                                                          |         |              | | **Downsample 1/4**           |
|                                                                                          |         |              | | **Downsample 1/8**           |
|                                                                                          |         |              | | **Downsample 1/16**          |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Ignore Alpha / ``Ignore_Alpha``                                                          | Boolean | Off          |                                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``                                               | Boolean | Off          |                                |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
| Log Verbosity / ``Log_Verbosity``                                                        | Choice  | Off          | |                              |
|                                                                                          |         |              | | **Off**                      |
|                                                                                          |         |              | | **Level 1**                  |
|                                                                                          |         |              | | **Level 2**                  |
|                                                                                          |         |              | | **Level 3**                  |
+------------------------------------------------------------------------------------------+---------+--------------+--------------------------------+
