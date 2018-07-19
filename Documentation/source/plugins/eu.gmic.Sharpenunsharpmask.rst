.. _eu.gmic.Sharpenunsharpmask:

G’MIC Sharpen unsharp mask node
===============================

*This documentation is for version 0.3 of G’MIC Sharpen unsharp mask.*

Description
-----------

Note:

This filter is inspired by the original GIMP Unsharp Mask filter, with additional parameters.

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

+--------------------------------------------+---------+-----------+-------------------------------------+
| Parameter / script name                    | Type    | Default   | Function                            |
+============================================+=========+===========+=====================================+
| Sharpening type / ``Sharpening_type``      | Choice  | Bilateral | |                                   |
|                                            |         |           | | **Gaussian**                      |
|                                            |         |           | | **Bilateral**                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Spatial radius / ``Spatial_radius``        | Double  | 1.25      |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Bilateral radius / ``Bilateral_radius``    | Double  | 10        |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Amount / ``Amount``                        | Double  | 2         |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Threshold / ``Threshold``                  | Double  | 0         |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Darkness level / ``Darkness_level``        | Double  | 1         |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Lightness level / ``Lightness_level``      | Double  | 1         |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Iterations / ``Iterations``                | Integer | 1         |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Negative effect / ``Negative_effect``      | Boolean | Off       |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Channel(s) / ``Channels``                  | Choice  | All       | |                                   |
|                                            |         |           | | **All**                           |
|                                            |         |           | | **RGBA [all]**                    |
|                                            |         |           | | **RGB [all]**                     |
|                                            |         |           | | **RGB [red]**                     |
|                                            |         |           | | **RGB [green]**                   |
|                                            |         |           | | **RGB [blue]**                    |
|                                            |         |           | | **RGBA [alpha]**                  |
|                                            |         |           | | **Linear RGB [all]**              |
|                                            |         |           | | **Linear RGB [red]**              |
|                                            |         |           | | **Linear RGB [green]**            |
|                                            |         |           | | **Linear RGB [blue]**             |
|                                            |         |           | | **YCbCr [luminance]**             |
|                                            |         |           | | **YCbCr [blue-red chrominances]** |
|                                            |         |           | | **YCbCr [blue chrominance]**      |
|                                            |         |           | | **YCbCr [red chrominance]**       |
|                                            |         |           | | **YCbCr [green chrominance]**     |
|                                            |         |           | | **Lab [lightness]**               |
|                                            |         |           | | **Lab [ab-chrominances]**         |
|                                            |         |           | | **Lab [a-chrominance]**           |
|                                            |         |           | | **Lab [b-chrominance]**           |
|                                            |         |           | | **Lch [ch-chrominances]**         |
|                                            |         |           | | **Lch [c-chrominance]**           |
|                                            |         |           | | **Lch [h-chrominance]**           |
|                                            |         |           | | **HSV [hue]**                     |
|                                            |         |           | | **HSV [saturation]**              |
|                                            |         |           | | **HSV [value]**                   |
|                                            |         |           | | **HSI [intensity]**               |
|                                            |         |           | | **HSL [lightness]**               |
|                                            |         |           | | **CMYK [cyan]**                   |
|                                            |         |           | | **CMYK [magenta]**                |
|                                            |         |           | | **CMYK [yellow]**                 |
|                                            |         |           | | **CMYK [key]**                    |
|                                            |         |           | | **YIQ [luma]**                    |
|                                            |         |           | | **YIQ [chromas]**                 |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Preview type / ``Preview_type``            | Choice  | Full      | |                                   |
|                                            |         |           | | **Full**                          |
|                                            |         |           | | **Forward horizontal**            |
|                                            |         |           | | **Forward vertical**              |
|                                            |         |           | | **Backward horizontal**           |
|                                            |         |           | | **Backward vertical**             |
|                                            |         |           | | **Duplicate top**                 |
|                                            |         |           | | **Duplicate left**                |
|                                            |         |           | | **Duplicate bottom**              |
|                                            |         |           | | **Duplicate right**               |
|                                            |         |           | | **Duplicate horizontal**          |
|                                            |         |           | | **Duplicate vertical**            |
|                                            |         |           | | **Checkered**                     |
|                                            |         |           | | **Checkered inverse)**            |
|                                            |         |           | | **Preview split = point(50**      |
|                                            |         |           | | **50**                            |
|                                            |         |           | | **0**                             |
|                                            |         |           | | **0**                             |
|                                            |         |           | | **200**                           |
|                                            |         |           | | **200**                           |
|                                            |         |           | | **200**                           |
|                                            |         |           | | **0**                             |
|                                            |         |           | | **10**                            |
|                                            |         |           | | **0**                             |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0   | |                                   |
|                                            |         |           | | **Merged**                        |
|                                            |         |           | | **Layer 0**                       |
|                                            |         |           | | **Layer 1**                       |
|                                            |         |           | | **Layer 2**                       |
|                                            |         |           | | **Layer 3**                       |
|                                            |         |           | | **Layer 4**                       |
|                                            |         |           | | **Layer 5**                       |
|                                            |         |           | | **Layer 6**                       |
|                                            |         |           | | **Layer 7**                       |
|                                            |         |           | | **Layer 8**                       |
|                                            |         |           | | **Layer 9**                       |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic   | |                                   |
|                                            |         |           | | **Fixed (Inplace)**               |
|                                            |         |           | | **Dynamic**                       |
|                                            |         |           | | **Downsample 1/2**                |
|                                            |         |           | | **Downsample 1/4**                |
|                                            |         |           | | **Downsample 1/8**                |
|                                            |         |           | | **Downsample 1/16**               |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off       |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off       |                                     |
+--------------------------------------------+---------+-----------+-------------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off       | |                                   |
|                                            |         |           | | **Off**                           |
|                                            |         |           | | **Level 1**                       |
|                                            |         |           | | **Level 2**                       |
|                                            |         |           | | **Level 3**                       |
+--------------------------------------------+---------+-----------+-------------------------------------+
