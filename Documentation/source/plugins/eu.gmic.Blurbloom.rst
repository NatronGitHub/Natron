.. _eu.gmic.Blurbloom:

G’MIC Blur bloom node
=====================

*This documentation is for version 0.3 of G’MIC Blur bloom.*

Description
-----------

Parameter Angle is only active when Anisotropy>0

Author: David Tschumperle. Latest update: 2015/03/02.

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

+--------------------------------------------+---------+------------------+-------------------------------------+
| Parameter / script name                    | Type    | Default          | Function                            |
+============================================+=========+==================+=====================================+
| Amplitude / ``Amplitude``                  | Double  | 1                |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Ratio / ``Ratio``                          | Double  | 2                |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Iterations / ``Iterations``                | Integer | 5                |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Operator / ``Operator``                    | Choice  | Add              | |                                   |
|                                            |         |                  | | **Add**                           |
|                                            |         |                  | | **Max**                           |
|                                            |         |                  | | **Min**                           |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Kernel / ``Kernel``                        | Choice  | Quasi-gaussian   | |                                   |
|                                            |         |                  | | **Quasi-gaussian**                |
|                                            |         |                  | | **Gaussian**                      |
|                                            |         |                  | | **Box**                           |
|                                            |         |                  | | **Triangle**                      |
|                                            |         |                  | | **Quadratic**                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Normalize scales / ``Normalize_scales``    | Boolean | Off              |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Anisotropy / ``Anisotropy``                | Double  | 0                |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Angle / ``Angle``                          | Double  | 0                |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Channel(s) / ``Channels``                  | Choice  | Linear RGB [all] | |                                   |
|                                            |         |                  | | **All**                           |
|                                            |         |                  | | **RGBA [all]**                    |
|                                            |         |                  | | **RGB [all]**                     |
|                                            |         |                  | | **RGB [red]**                     |
|                                            |         |                  | | **RGB [green]**                   |
|                                            |         |                  | | **RGB [blue]**                    |
|                                            |         |                  | | **RGBA [alpha]**                  |
|                                            |         |                  | | **Linear RGB [all]**              |
|                                            |         |                  | | **Linear RGB [red]**              |
|                                            |         |                  | | **Linear RGB [green]**            |
|                                            |         |                  | | **Linear RGB [blue]**             |
|                                            |         |                  | | **YCbCr [luminance]**             |
|                                            |         |                  | | **YCbCr [blue-red chrominances]** |
|                                            |         |                  | | **YCbCr [blue chrominance]**      |
|                                            |         |                  | | **YCbCr [red chrominance]**       |
|                                            |         |                  | | **YCbCr [green chrominance]**     |
|                                            |         |                  | | **Lab [lightness]**               |
|                                            |         |                  | | **Lab [ab-chrominances]**         |
|                                            |         |                  | | **Lab [a-chrominance]**           |
|                                            |         |                  | | **Lab [b-chrominance]**           |
|                                            |         |                  | | **Lch [ch-chrominances]**         |
|                                            |         |                  | | **Lch [c-chrominance]**           |
|                                            |         |                  | | **Lch [h-chrominance]**           |
|                                            |         |                  | | **HSV [hue]**                     |
|                                            |         |                  | | **HSV [saturation]**              |
|                                            |         |                  | | **HSV [value]**                   |
|                                            |         |                  | | **HSI [intensity]**               |
|                                            |         |                  | | **HSL [lightness]**               |
|                                            |         |                  | | **CMYK [cyan]**                   |
|                                            |         |                  | | **CMYK [magenta]**                |
|                                            |         |                  | | **CMYK [yellow]**                 |
|                                            |         |                  | | **CMYK [key]**                    |
|                                            |         |                  | | **YIQ [luma]**                    |
|                                            |         |                  | | **YIQ [chromas]**                 |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Preview type / ``Preview_type``            | Choice  | Full             | |                                   |
|                                            |         |                  | | **Full**                          |
|                                            |         |                  | | **Forward horizontal**            |
|                                            |         |                  | | **Forward vertical**              |
|                                            |         |                  | | **Backward horizontal**           |
|                                            |         |                  | | **Backward vertical**             |
|                                            |         |                  | | **Duplicate top**                 |
|                                            |         |                  | | **Duplicate left**                |
|                                            |         |                  | | **Duplicate bottom**              |
|                                            |         |                  | | **Duplicate right**               |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0          | |                                   |
|                                            |         |                  | | **Merged**                        |
|                                            |         |                  | | **Layer 0**                       |
|                                            |         |                  | | **Layer 1**                       |
|                                            |         |                  | | **Layer 2**                       |
|                                            |         |                  | | **Layer 3**                       |
|                                            |         |                  | | **Layer 4**                       |
|                                            |         |                  | | **Layer 5**                       |
|                                            |         |                  | | **Layer 6**                       |
|                                            |         |                  | | **Layer 7**                       |
|                                            |         |                  | | **Layer 8**                       |
|                                            |         |                  | | **Layer 9**                       |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic          | |                                   |
|                                            |         |                  | | **Fixed (Inplace)**               |
|                                            |         |                  | | **Dynamic**                       |
|                                            |         |                  | | **Downsample 1/2**                |
|                                            |         |                  | | **Downsample 1/4**                |
|                                            |         |                  | | **Downsample 1/8**                |
|                                            |         |                  | | **Downsample 1/16**               |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off              |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off              |                                     |
+--------------------------------------------+---------+------------------+-------------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off              | |                                   |
|                                            |         |                  | | **Off**                           |
|                                            |         |                  | | **Level 1**                       |
|                                            |         |                  | | **Level 2**                       |
|                                            |         |                  | | **Level 3**                       |
+--------------------------------------------+---------+------------------+-------------------------------------+
