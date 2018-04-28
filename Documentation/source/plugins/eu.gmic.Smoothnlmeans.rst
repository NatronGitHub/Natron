.. _eu.gmic.Smoothnlmeans:

G’MIC Smooth nlmeans node
=========================

*This documentation is for version 0.3 of G’MIC Smooth nlmeans.*

Description
-----------

Author: Jerome Boulanger. Latest update: 2015/01/07.

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

+-----------------------------------------------+---------+-----------+-------------------------------------+
| Parameter / script name                       | Type    | Default   | Function                            |
+===============================================+=========+===========+=====================================+
| Patch size / ``Patch_size``                   | Double  | 4         |                                     |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Spatial bandwidth / ``Spatial_bandwidth``     | Integer | 4         |                                     |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Tonal bandwidth / ``Tonal_bandwidth``         | Double  | 10        |                                     |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Patch measure / ``Patch_measure``             | Choice  | Luminance | |                                   |
|                                               |         |           | | **Linf-norm**                     |
|                                               |         |           | | **L1-norm**                       |
|                                               |         |           | | **L2-norm**                       |
|                                               |         |           | | **Luminance**                     |
|                                               |         |           | | **Lightness**                     |
|                                               |         |           | | **RGB**                           |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Channel(s) / ``Channels``                     | Choice  | All       | |                                   |
|                                               |         |           | | **All**                           |
|                                               |         |           | | **RGBA [all]**                    |
|                                               |         |           | | **RGB [all]**                     |
|                                               |         |           | | **RGB [red]**                     |
|                                               |         |           | | **RGB [green]**                   |
|                                               |         |           | | **RGB [blue]**                    |
|                                               |         |           | | **RGBA [alpha]**                  |
|                                               |         |           | | **Linear RGB [all]**              |
|                                               |         |           | | **Linear RGB [red]**              |
|                                               |         |           | | **Linear RGB [green]**            |
|                                               |         |           | | **Linear RGB [blue]**             |
|                                               |         |           | | **YCbCr [luminance]**             |
|                                               |         |           | | **YCbCr [blue-red chrominances]** |
|                                               |         |           | | **YCbCr [blue chrominance]**      |
|                                               |         |           | | **YCbCr [red chrominance]**       |
|                                               |         |           | | **YCbCr [green chrominance]**     |
|                                               |         |           | | **Lab [lightness]**               |
|                                               |         |           | | **Lab [ab-chrominances]**         |
|                                               |         |           | | **Lab [a-chrominance]**           |
|                                               |         |           | | **Lab [b-chrominance]**           |
|                                               |         |           | | **Lch [ch-chrominances]**         |
|                                               |         |           | | **Lch [c-chrominance]**           |
|                                               |         |           | | **Lch [h-chrominance]**           |
|                                               |         |           | | **HSV [hue]**                     |
|                                               |         |           | | **HSV [saturation]**              |
|                                               |         |           | | **HSV [value]**                   |
|                                               |         |           | | **HSI [intensity]**               |
|                                               |         |           | | **HSL [lightness]**               |
|                                               |         |           | | **CMYK [cyan]**                   |
|                                               |         |           | | **CMYK [magenta]**                |
|                                               |         |           | | **CMYK [yellow]**                 |
|                                               |         |           | | **CMYK [key]**                    |
|                                               |         |           | | **YIQ [luma]**                    |
|                                               |         |           | | **YIQ [chromas]**                 |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Parallel processing / ``Parallel_processing`` | Choice  | Auto      | |                                   |
|                                               |         |           | | **Auto**                          |
|                                               |         |           | | **One thread**                    |
|                                               |         |           | | **Two threads**                   |
|                                               |         |           | | **Four threads**                  |
|                                               |         |           | | **Eight threads**                 |
|                                               |         |           | | **Sixteen threads)**              |
|                                               |         |           | | **Spatial overlap = int(24**      |
|                                               |         |           | | **0**                             |
|                                               |         |           | | **256**                           |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Preview type / ``Preview_type``               | Choice  | Full      | |                                   |
|                                               |         |           | | **Full**                          |
|                                               |         |           | | **Forward horizontal**            |
|                                               |         |           | | **Forward vertical**              |
|                                               |         |           | | **Backward horizontal**           |
|                                               |         |           | | **Backward vertical**             |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Output Layer / ``Output_Layer``               | Choice  | Layer 0   | |                                   |
|                                               |         |           | | **Merged**                        |
|                                               |         |           | | **Layer 0**                       |
|                                               |         |           | | **Layer 1**                       |
|                                               |         |           | | **Layer 2**                       |
|                                               |         |           | | **Layer 3**                       |
|                                               |         |           | | **Layer 4**                       |
|                                               |         |           | | **Layer 5**                       |
|                                               |         |           | | **Layer 6**                       |
|                                               |         |           | | **Layer 7**                       |
|                                               |         |           | | **Layer 8**                       |
|                                               |         |           | | **Layer 9**                       |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Resize Mode / ``Resize_Mode``                 | Choice  | Dynamic   | |                                   |
|                                               |         |           | | **Fixed (Inplace)**               |
|                                               |         |           | | **Dynamic**                       |
|                                               |         |           | | **Downsample 1/2**                |
|                                               |         |           | | **Downsample 1/4**                |
|                                               |         |           | | **Downsample 1/8**                |
|                                               |         |           | | **Downsample 1/16**               |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``               | Boolean | Off       |                                     |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``    | Boolean | Off       |                                     |
+-----------------------------------------------+---------+-----------+-------------------------------------+
| Log Verbosity / ``Log_Verbosity``             | Choice  | Off       | |                                   |
|                                               |         |           | | **Off**                           |
|                                               |         |           | | **Level 1**                       |
|                                               |         |           | | **Level 2**                       |
|                                               |         |           | | **Level 3**                       |
+-----------------------------------------------+---------+-----------+-------------------------------------+
