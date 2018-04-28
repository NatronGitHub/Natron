.. _eu.gmic.Instantpro:

G’MIC Instant pro node
======================

*This documentation is for version 0.3 of G’MIC Instant pro.*

Description
-----------

Note: The color LUTs used in this section have been designed by Patrick David. More info at:

Film Emulation Presets in G’MIC: http://gmic.eu/film_emulation/index.shtml

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

+--------------------------------------------+---------+---------+------------------------------------+
| Parameter / script name                    | Type    | Default | Function                           |
+============================================+=========+=========+====================================+
| Preset / ``Preset``                        | Choice  | None    | |                                  |
|                                            |         |         | | **None**                         |
|                                            |         |         | | **Fuji FP-100c --**              |
|                                            |         |         | | **Fuji FP-100c -**               |
|                                            |         |         | | **Fuji FP-100c**                 |
|                                            |         |         | | **Fuji FP-100c +**               |
|                                            |         |         | | **Fuji FP-100c ++**              |
|                                            |         |         | | **Fuji FP-100c ++a**             |
|                                            |         |         | | **Fuji FP-100c +++**             |
|                                            |         |         | | **Fuji FP-100c Cool --**         |
|                                            |         |         | | **Fuji FP-100c Cool -**          |
|                                            |         |         | | **Fuji FP-100c Cool**            |
|                                            |         |         | | **Fuji FP-100c Cool +**          |
|                                            |         |         | | **Fuji FP-100c Cool ++**         |
|                                            |         |         | | **Fuji FP-100c Negative --**     |
|                                            |         |         | | **Fuji FP-100c Negative -**      |
|                                            |         |         | | **Fuji FP-100c Negative**        |
|                                            |         |         | | **Fuji FP-100c Negative +**      |
|                                            |         |         | | **Fuji FP-100c Negative ++**     |
|                                            |         |         | | **Fuji FP-100c Negative ++a**    |
|                                            |         |         | | **Fuji FP-100c Negative +++**    |
|                                            |         |         | | **Fuji FP-3000b --**             |
|                                            |         |         | | **Fuji FP-3000b -**              |
|                                            |         |         | | **Fuji FP-3000b**                |
|                                            |         |         | | **Fuji FP-3000b +**              |
|                                            |         |         | | **Fuji FP-3000b ++**             |
|                                            |         |         | | **Fuji FP-3000b +++**            |
|                                            |         |         | | **Fuji FP-3000b HC**             |
|                                            |         |         | | **Fuji FP-3000b Negative --**    |
|                                            |         |         | | **Fuji FP-3000b Negative -**     |
|                                            |         |         | | **Fuji FP-3000b Negative**       |
|                                            |         |         | | **Fuji FP-3000b Negative +**     |
|                                            |         |         | | **Fuji FP-3000b Negative ++**    |
|                                            |         |         | | **Fuji FP-3000b Negative +++**   |
|                                            |         |         | | **Fuji FP-3000b Negative Early** |
|                                            |         |         | | **Polaroid 665 --**              |
|                                            |         |         | | **Polaroid 665 -**               |
|                                            |         |         | | **Polaroid 665**                 |
|                                            |         |         | | **Polaroid 665 +**               |
|                                            |         |         | | **Polaroid 665 ++**              |
|                                            |         |         | | **Polaroid 665 Negative -**      |
|                                            |         |         | | **Polaroid 665 Negative**        |
|                                            |         |         | | **Polaroid 665 Negative +**      |
|                                            |         |         | | **Polaroid 665 Negative HC**     |
|                                            |         |         | | **Polaroid 669 --**              |
|                                            |         |         | | **Polaroid 669 -**               |
|                                            |         |         | | **Polaroid 669**                 |
|                                            |         |         | | **Polaroid 669 +**               |
|                                            |         |         | | **Polaroid 669 ++**              |
|                                            |         |         | | **Polaroid 669 +++**             |
|                                            |         |         | | **Polaroid 669 Cold --**         |
|                                            |         |         | | **Polaroid 669 Cold -**          |
|                                            |         |         | | **Polaroid 669 Cold**            |
|                                            |         |         | | **Polaroid 669 Cold +**          |
|                                            |         |         | | **Polaroid 690 --**              |
|                                            |         |         | | **Polaroid 690 -**               |
|                                            |         |         | | **Polaroid 690**                 |
|                                            |         |         | | **Polaroid 690 +**               |
|                                            |         |         | | **Polaroid 690 ++**              |
|                                            |         |         | | **Polaroid 690 Cold --**         |
|                                            |         |         | | **Polaroid 690 Cold -**          |
|                                            |         |         | | **Polaroid 690 Cold**            |
|                                            |         |         | | **Polaroid 690 Cold +**          |
|                                            |         |         | | **Polaroid 690 Cold ++**         |
|                                            |         |         | | **Polaroid 690 Warm --**         |
|                                            |         |         | | **Polaroid 690 Warm -**          |
|                                            |         |         | | **Polaroid 690 Warm**            |
|                                            |         |         | | **Polaroid 690 Warm +**          |
|                                            |         |         | | **Polaroid 690 Warm ++**         |
+--------------------------------------------+---------+---------+------------------------------------+
| Strength (%) / ``Strength_``               | Double  | 100     |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Brightness (%) / ``Brightness_``           | Double  | 0       |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Contrast (%) / ``Contrast_``               | Double  | 0       |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Gamma (%) / ``Gamma_``                     | Double  | 0       |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Hue (%) / ``Hue_``                         | Double  | 0       |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Saturation (%) / ``Saturation_``           | Double  | 0       |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Normalize colors / ``Normalize_colors``    | Choice  | None    | |                                  |
|                                            |         |         | | **None**                         |
|                                            |         |         | | **Pre-process**                  |
|                                            |         |         | | **Post-process**                 |
|                                            |         |         | | **Both**                         |
+--------------------------------------------+---------+---------+------------------------------------+
| Preview type / ``Preview_type``            | Choice  | Full    | |                                  |
|                                            |         |         | | **Full**                         |
|                                            |         |         | | **Forward horizontal**           |
|                                            |         |         | | **Forward vertical**             |
|                                            |         |         | | **Backward horizontal**          |
|                                            |         |         | | **Backward vertical**            |
|                                            |         |         | | **Duplicate top**                |
|                                            |         |         | | **Duplicate left**               |
|                                            |         |         | | **Duplicate bottom**             |
|                                            |         |         | | **Duplicate right**              |
+--------------------------------------------+---------+---------+------------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                                  |
|                                            |         |         | | **Merged**                       |
|                                            |         |         | | **Layer 0**                      |
|                                            |         |         | | **Layer 1**                      |
|                                            |         |         | | **Layer 2**                      |
|                                            |         |         | | **Layer 3**                      |
|                                            |         |         | | **Layer 4**                      |
|                                            |         |         | | **Layer 5**                      |
|                                            |         |         | | **Layer 6**                      |
|                                            |         |         | | **Layer 7**                      |
|                                            |         |         | | **Layer 8**                      |
|                                            |         |         | | **Layer 9**                      |
+--------------------------------------------+---------+---------+------------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                                  |
|                                            |         |         | | **Fixed (Inplace)**              |
|                                            |         |         | | **Dynamic**                      |
|                                            |         |         | | **Downsample 1/2**               |
|                                            |         |         | | **Downsample 1/4**               |
|                                            |         |         | | **Downsample 1/8**               |
|                                            |         |         | | **Downsample 1/16**              |
+--------------------------------------------+---------+---------+------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                                    |
+--------------------------------------------+---------+---------+------------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                                  |
|                                            |         |         | | **Off**                          |
|                                            |         |         | | **Level 1**                      |
|                                            |         |         | | **Level 2**                      |
|                                            |         |         | | **Level 3**                      |
+--------------------------------------------+---------+---------+------------------------------------+
