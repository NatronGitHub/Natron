.. _eu.gmic.PictureFX:

G’MIC PictureFX node
====================

*This documentation is for version 0.3 of G’MIC PictureFX.*

Description
-----------

Note: The color LUTs used in this section have been provided by Marc Roovers, and are freely available at:

PictureFX - a free HaldCLUT set: http://www.digicrea.be/haldclut-set-style-a-la-nik-software

Authors: Marc Roovers and David Tschumperle. Latest update: 2016/21/10.

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

+--------------------------------------------+---------+---------+--------------------------------------+
| Parameter / script name                    | Type    | Default | Function                             |
+============================================+=========+=========+======================================+
| Preset / ``Preset``                        | Choice  | None    | |                                    |
|                                            |         |         | | **None**                           |
|                                            |         |         | | **AnalogFX - Anno 1870 Color**     |
|                                            |         |         | | **AnalogFX - Old Style I**         |
|                                            |         |         | | **AnalogFX - Old Style II**        |
|                                            |         |         | | **AnalogFX - Old Style III**       |
|                                            |         |         | | **AnalogFX - Sepia Color**         |
|                                            |         |         | | **AnalogFX - Soft Sepia I**        |
|                                            |         |         | | **AnalogFX - Soft Sepia II**       |
|                                            |         |         | | **GoldFX - Perfect Sunset 01min**  |
|                                            |         |         | | **GoldFX - Perfect Sunset 05min**  |
|                                            |         |         | | **GoldFX - Perfect Sunset 10min**  |
|                                            |         |         | | **GoldFX - Spring breeze**         |
|                                            |         |         | | **GoldFX - Bright spring breeze**  |
|                                            |         |         | | **GoldFX - Summer heat**           |
|                                            |         |         | | **GoldFX - Bright summer heat**    |
|                                            |         |         | | **GoldFX - Hot summer heat**       |
|                                            |         |         | | **TechnicalFX - Backlight filter** |
|                                            |         |         | | **ZilverFX - B&W Solarization**    |
|                                            |         |         | | **ZilverFX - Infrared**            |
|                                            |         |         | | **ZilverFX - Vintage B&W**         |
+--------------------------------------------+---------+---------+--------------------------------------+
| Strength (%) / ``Strength_``               | Double  | 100     |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Brightness (%) / ``Brightness_``           | Double  | 0       |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Contrast (%) / ``Contrast_``               | Double  | 0       |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Gamma (%) / ``Gamma_``                     | Double  | 0       |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Hue (%) / ``Hue_``                         | Double  | 0       |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Saturation (%) / ``Saturation_``           | Double  | 0       |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Normalize colors / ``Normalize_colors``    | Choice  | None    | |                                    |
|                                            |         |         | | **None**                           |
|                                            |         |         | | **Pre-process**                    |
|                                            |         |         | | **Post-process**                   |
|                                            |         |         | | **Both**                           |
+--------------------------------------------+---------+---------+--------------------------------------+
| Preview type / ``Preview_type``            | Choice  | Full    | |                                    |
|                                            |         |         | | **Full**                           |
|                                            |         |         | | **Forward horizontal**             |
|                                            |         |         | | **Forward vertical**               |
|                                            |         |         | | **Backward horizontal**            |
|                                            |         |         | | **Backward vertical**              |
|                                            |         |         | | **Duplicate top**                  |
|                                            |         |         | | **Duplicate left**                 |
|                                            |         |         | | **Duplicate bottom**               |
|                                            |         |         | | **Duplicate right**                |
+--------------------------------------------+---------+---------+--------------------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                                    |
|                                            |         |         | | **Merged**                         |
|                                            |         |         | | **Layer 0**                        |
|                                            |         |         | | **Layer 1**                        |
|                                            |         |         | | **Layer 2**                        |
|                                            |         |         | | **Layer 3**                        |
|                                            |         |         | | **Layer 4**                        |
|                                            |         |         | | **Layer 5**                        |
|                                            |         |         | | **Layer 6**                        |
|                                            |         |         | | **Layer 7**                        |
|                                            |         |         | | **Layer 8**                        |
|                                            |         |         | | **Layer 9**                        |
+--------------------------------------------+---------+---------+--------------------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                                    |
|                                            |         |         | | **Fixed (Inplace)**                |
|                                            |         |         | | **Dynamic**                        |
|                                            |         |         | | **Downsample 1/2**                 |
|                                            |         |         | | **Downsample 1/4**                 |
|                                            |         |         | | **Downsample 1/8**                 |
|                                            |         |         | | **Downsample 1/16**                |
+--------------------------------------------+---------+---------+--------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                                      |
+--------------------------------------------+---------+---------+--------------------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                                    |
|                                            |         |         | | **Off**                            |
|                                            |         |         | | **Level 1**                        |
|                                            |         |         | | **Level 2**                        |
|                                            |         |         | | **Level 3**                        |
+--------------------------------------------+---------+---------+--------------------------------------+
