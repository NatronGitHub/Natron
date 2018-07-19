.. _eu.gmic.Slidecolor:

G’MIC Slide color node
======================

*This documentation is for version 0.3 of G’MIC Slide color.*

Description
-----------

Note: The color LUTs used in this section have been designed by Patrick David. More info at:

Film Emulation Presets in G’MIC: https://gmic.eu/film_emulation/index.shtml

Authors: Patrick David and David Tschumperle. Latest update: 2016/02/08.

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

+--------------------------------------------+---------+---------+---------------------------------------+
| Parameter / script name                    | Type    | Default | Function                              |
+============================================+=========+=========+=======================================+
| Preset / ``Preset``                        | Choice  | None    | |                                     |
|                                            |         |         | | **None**                            |
|                                            |         |         | | **Agfa Precisa 100**                |
|                                            |         |         | | **Fuji Astia 100F**                 |
|                                            |         |         | | **Fuji FP 100C**                    |
|                                            |         |         | | **Fuji Provia 100F**                |
|                                            |         |         | | **Fuji Provia 400F**                |
|                                            |         |         | | **Fuji Provia 400X**                |
|                                            |         |         | | **Fuji Sensia 100**                 |
|                                            |         |         | | **Fuji Superia 200 XPRO**           |
|                                            |         |         | | **Fuji Velvia 50**                  |
|                                            |         |         | | **Generic Fuji Astia 100**          |
|                                            |         |         | | **Generic Fuji Provia 100**         |
|                                            |         |         | | **Generic Fuji Velvia 100**         |
|                                            |         |         | | **Generic Kodachrome 64**           |
|                                            |         |         | | **Generic Kodak Ektachrome 100 VS** |
|                                            |         |         | | **Kodak E-100 GX Ektachrome 100**   |
|                                            |         |         | | **Kodak Ektachrome 100 VS**         |
|                                            |         |         | | **Kodak Elite Chrome 200**          |
|                                            |         |         | | **Kodak Elite Chrome 400**          |
|                                            |         |         | | **Kodak Elite ExtraColor 100**      |
|                                            |         |         | | **Kodak Kodachrome 200**            |
|                                            |         |         | | **Kodak Kodachrome 25**             |
|                                            |         |         | | **Kodak Kodachrome 64**             |
|                                            |         |         | | **Lomography X-Pro Slide 200**      |
|                                            |         |         | | **Polaroid 669**                    |
|                                            |         |         | | **Polaroid 690**                    |
|                                            |         |         | | **Polaroid Polachrome**             |
+--------------------------------------------+---------+---------+---------------------------------------+
| Strength (%) / ``Strength_``               | Double  | 100     |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Brightness (%) / ``Brightness_``           | Double  | 0       |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Contrast (%) / ``Contrast_``               | Double  | 0       |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Gamma (%) / ``Gamma_``                     | Double  | 0       |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Hue (%) / ``Hue_``                         | Double  | 0       |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Saturation (%) / ``Saturation_``           | Double  | 0       |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Normalize colors / ``Normalize_colors``    | Choice  | None    | |                                     |
|                                            |         |         | | **None**                            |
|                                            |         |         | | **Pre-process**                     |
|                                            |         |         | | **Post-process**                    |
|                                            |         |         | | **Both**                            |
+--------------------------------------------+---------+---------+---------------------------------------+
| Preview type / ``Preview_type``            | Choice  | Full    | |                                     |
|                                            |         |         | | **Full**                            |
|                                            |         |         | | **Forward horizontal**              |
|                                            |         |         | | **Forward vertical**                |
|                                            |         |         | | **Backward horizontal**             |
|                                            |         |         | | **Backward vertical**               |
|                                            |         |         | | **Duplicate top**                   |
|                                            |         |         | | **Duplicate left**                  |
|                                            |         |         | | **Duplicate bottom**                |
|                                            |         |         | | **Duplicate right**                 |
|                                            |         |         | | **Duplicate horizontal**            |
|                                            |         |         | | **Duplicate vertical**              |
|                                            |         |         | | **Checkered**                       |
|                                            |         |         | | **Checkered inverse)**              |
|                                            |         |         | | **Preview split = point(50**        |
|                                            |         |         | | **50**                              |
|                                            |         |         | | **0**                               |
|                                            |         |         | | **0**                               |
|                                            |         |         | | **200**                             |
|                                            |         |         | | **200**                             |
|                                            |         |         | | **200**                             |
|                                            |         |         | | **0**                               |
|                                            |         |         | | **10**                              |
|                                            |         |         | | **0**                               |
+--------------------------------------------+---------+---------+---------------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                                     |
|                                            |         |         | | **Merged**                          |
|                                            |         |         | | **Layer 0**                         |
|                                            |         |         | | **Layer 1**                         |
|                                            |         |         | | **Layer 2**                         |
|                                            |         |         | | **Layer 3**                         |
|                                            |         |         | | **Layer 4**                         |
|                                            |         |         | | **Layer 5**                         |
|                                            |         |         | | **Layer 6**                         |
|                                            |         |         | | **Layer 7**                         |
|                                            |         |         | | **Layer 8**                         |
|                                            |         |         | | **Layer 9**                         |
+--------------------------------------------+---------+---------+---------------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                                     |
|                                            |         |         | | **Fixed (Inplace)**                 |
|                                            |         |         | | **Dynamic**                         |
|                                            |         |         | | **Downsample 1/2**                  |
|                                            |         |         | | **Downsample 1/4**                  |
|                                            |         |         | | **Downsample 1/8**                  |
|                                            |         |         | | **Downsample 1/16**                 |
+--------------------------------------------+---------+---------+---------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                                       |
+--------------------------------------------+---------+---------+---------------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                                     |
|                                            |         |         | | **Off**                             |
|                                            |         |         | | **Level 1**                         |
|                                            |         |         | | **Level 2**                         |
|                                            |         |         | | **Level 3**                         |
+--------------------------------------------+---------+---------+---------------------------------------+
