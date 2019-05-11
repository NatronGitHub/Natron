.. _eu.gmic.Blendstandard:

G’MIC Blend standard node
=========================

*This documentation is for version 1.0 of G’MIC Blend standard.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

+----------+-------------+----------+
| Input    | Description | Optional |
+==========+=============+==========+
| Source   |             | No       |
+----------+-------------+----------+
| Layer -1 |             | Yes      |
+----------+-------------+----------+
| Layer -2 |             | Yes      |
+----------+-------------+----------+
| Layer -3 |             | Yes      |
+----------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Parameter / script name                       | Type    | Default                             | Function                                             |
+===============================================+=========+=====================================+======================================================+
| Mode / ``Mode``                               | Choice  | Custom formula                      | |                                                    |
|                                               |         |                                     | | **Add**                                            |
|                                               |         |                                     | | **Alpha**                                          |
|                                               |         |                                     | | **And**                                            |
|                                               |         |                                     | | **Average**                                        |
|                                               |         |                                     | | **Blue**                                           |
|                                               |         |                                     | | **Burn**                                           |
|                                               |         |                                     | | **Custom formula**                                 |
|                                               |         |                                     | | **Darken**                                         |
|                                               |         |                                     | | **Difference**                                     |
|                                               |         |                                     | | **Divide**                                         |
|                                               |         |                                     | | **Dodge**                                          |
|                                               |         |                                     | | **Edges**                                          |
|                                               |         |                                     | | **Exclusion**                                      |
|                                               |         |                                     | | **Freeze**                                         |
|                                               |         |                                     | | **Grain extract**                                  |
|                                               |         |                                     | | **Grain merge**                                    |
|                                               |         |                                     | | **Green**                                          |
|                                               |         |                                     | | **Hard light**                                     |
|                                               |         |                                     | | **Hard mix**                                       |
|                                               |         |                                     | | **Hue**                                            |
|                                               |         |                                     | | **Interpolation**                                  |
|                                               |         |                                     | | **Lighten**                                        |
|                                               |         |                                     | | **Lightness**                                      |
|                                               |         |                                     | | **Linear burn**                                    |
|                                               |         |                                     | | **Linear light**                                   |
|                                               |         |                                     | | **Luminance**                                      |
|                                               |         |                                     | | **Multiply**                                       |
|                                               |         |                                     | | **Negation**                                       |
|                                               |         |                                     | | **Or**                                             |
|                                               |         |                                     | | **Overlay**                                        |
|                                               |         |                                     | | **Pin light**                                      |
|                                               |         |                                     | | **Red**                                            |
|                                               |         |                                     | | **Reflect**                                        |
|                                               |         |                                     | | **Saturation**                                     |
|                                               |         |                                     | | **Shape area max**                                 |
|                                               |         |                                     | | **Shape area max0**                                |
|                                               |         |                                     | | **Shape area min**                                 |
|                                               |         |                                     | | **Shape area min0**                                |
|                                               |         |                                     | | **Shape average**                                  |
|                                               |         |                                     | | **Shape average0**                                 |
|                                               |         |                                     | | **Shape median**                                   |
|                                               |         |                                     | | **Shape median0**                                  |
|                                               |         |                                     | | **Shape min**                                      |
|                                               |         |                                     | | **Shape min0**                                     |
|                                               |         |                                     | | **Shape max**                                      |
|                                               |         |                                     | | **Shape max0**                                     |
|                                               |         |                                     | | **Soft burn**                                      |
|                                               |         |                                     | | **Soft dodge**                                     |
|                                               |         |                                     | | **Soft light**                                     |
|                                               |         |                                     | | **Screen**                                         |
|                                               |         |                                     | | **Stamp**                                          |
|                                               |         |                                     | | **Subtract**                                       |
|                                               |         |                                     | | **Value**                                          |
|                                               |         |                                     | | **Vivid light**                                    |
|                                               |         |                                     | | **Xor**                                            |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Process as / ``Process_as``                   | Choice  | Two-by-two                          | |                                                    |
|                                               |         |                                     | | **Two-by-two**                                     |
|                                               |         |                                     | | **Upper layer is the top layer for all blends**    |
|                                               |         |                                     | | **Lower layer is the bottom layer for all blends** |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Opacity (%) / ``Opacity_``                    | Double  | 100                                 |                                                      |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Preview all outputs / ``Preview_all_outputs`` | Boolean | On                                  |                                                      |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Custom formula / ``Custom_formula``           | String  | 1/2 - 1/4*cos(pi*a) - 1/4*cos(pi*b) |                                                      |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Output Layer / ``Output_Layer``               | Choice  | Layer 0                             | |                                                    |
|                                               |         |                                     | | **Merged**                                         |
|                                               |         |                                     | | **Layer 0**                                        |
|                                               |         |                                     | | **Layer -1**                                       |
|                                               |         |                                     | | **Layer -2**                                       |
|                                               |         |                                     | | **Layer -3**                                       |
|                                               |         |                                     | | **Layer -4**                                       |
|                                               |         |                                     | | **Layer -5**                                       |
|                                               |         |                                     | | **Layer -6**                                       |
|                                               |         |                                     | | **Layer -7**                                       |
|                                               |         |                                     | | **Layer -8**                                       |
|                                               |         |                                     | | **Layer -9**                                       |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Resize Mode / ``Resize_Mode``                 | Choice  | Dynamic                             | |                                                    |
|                                               |         |                                     | | **Fixed (Inplace)**                                |
|                                               |         |                                     | | **Dynamic**                                        |
|                                               |         |                                     | | **Downsample 1/2**                                 |
|                                               |         |                                     | | **Downsample 1/4**                                 |
|                                               |         |                                     | | **Downsample 1/8**                                 |
|                                               |         |                                     | | **Downsample 1/16**                                |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``               | Boolean | Off                                 |                                                      |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``    | Boolean | Off                                 |                                                      |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
| Log Verbosity / ``Log_Verbosity``             | Choice  | Off                                 | |                                                    |
|                                               |         |                                     | | **Off**                                            |
|                                               |         |                                     | | **Level 1**                                        |
|                                               |         |                                     | | **Level 2**                                        |
|                                               |         |                                     | | **Level 3**                                        |
+-----------------------------------------------+---------+-------------------------------------+------------------------------------------------------+
