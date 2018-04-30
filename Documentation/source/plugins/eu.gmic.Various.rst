.. _eu.gmic.Various:

G’MIC Various node
==================

*This documentation is for version 0.3 of G’MIC Various.*

Description
-----------

More info at:

Film Emulation Presets in G’MIC: http://gmic.eu/film_emulation/index.shtml

Authors: David Tschumperle and Patrick David. Latest update: 2016/02/08.

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

+--------------------------------------------+---------+---------+---------------------------+
| Parameter / script name                    | Type    | Default | Function                  |
+============================================+=========+=========+===========================+
| Preset / ``Preset``                        | Choice  | None    | |                         |
|                                            |         |         | | **None**                |
|                                            |         |         | | **60’s**                |
|                                            |         |         | | **60’s (faded)**        |
|                                            |         |         | | **60’s (faded alt)**    |
|                                            |         |         | | **Alien green**         |
|                                            |         |         | | **Black & White**       |
|                                            |         |         | | **Bleach bypass**       |
|                                            |         |         | | **Blue mono**           |
|                                            |         |         | | **Color (rich)**        |
|                                            |         |         | | **Faded**               |
|                                            |         |         | | **Faded (alt)**         |
|                                            |         |         | | **Faded (analog)**      |
|                                            |         |         | | **Faded (extreme)**     |
|                                            |         |         | | **Faded (vivid)**       |
|                                            |         |         | | **Expired (fade)**      |
|                                            |         |         | | **Expired (polaroid)**  |
|                                            |         |         | | **Extreme**             |
|                                            |         |         | | **Fade**                |
|                                            |         |         | | **Faux infrared**       |
|                                            |         |         | | **Golden**              |
|                                            |         |         | | **Golden (bright)**     |
|                                            |         |         | | **Golden (fade)**       |
|                                            |         |         | | **Golden (mono)**       |
|                                            |         |         | | **Golden (vibrant)**    |
|                                            |         |         | | **Green mono**          |
|                                            |         |         | | **Hong Kong**           |
|                                            |         |         | | **Light (blown)**       |
|                                            |         |         | | **Lomo**                |
|                                            |         |         | | **Mono tinted**         |
|                                            |         |         | | **Muted fade**          |
|                                            |         |         | | **Mute shift**          |
|                                            |         |         | | **Natural (vivid)**     |
|                                            |         |         | | **Nostalgic**           |
|                                            |         |         | | **Orange tone**         |
|                                            |         |         | | **Pink fade**           |
|                                            |         |         | | **Purple**              |
|                                            |         |         | | **Retro**               |
|                                            |         |         | | **Rotate (muted)**      |
|                                            |         |         | | **Rotate (vibrant)**    |
|                                            |         |         | | **Smooth crome-ish**    |
|                                            |         |         | | **Smooth fade**         |
|                                            |         |         | | **Soft fade**           |
|                                            |         |         | | **Solarize color**      |
|                                            |         |         | | **Solarized color2**    |
|                                            |         |         | | **Summer**              |
|                                            |         |         | | **Summer (alt)**        |
|                                            |         |         | | **Sunny**               |
|                                            |         |         | | **Sunny (alt)**         |
|                                            |         |         | | **Sunny (warm)**        |
|                                            |         |         | | **Sunny (rich)**        |
|                                            |         |         | | **Super warm**          |
|                                            |         |         | | **Super warm (rich)**   |
|                                            |         |         | | **Sutro FX**            |
|                                            |         |         | | **Vibrant**             |
|                                            |         |         | | **Vibrant (alien)**     |
|                                            |         |         | | **Vibrant (contrast)**  |
|                                            |         |         | | **Vibrant (crome-ish)** |
|                                            |         |         | | **Vintage**             |
|                                            |         |         | | **Vintage (alt)**       |
|                                            |         |         | | **Vintage (brighter)**  |
|                                            |         |         | | **Warm**                |
|                                            |         |         | | **Warm (highlight)**    |
|                                            |         |         | | **Warm (yellow)**       |
+--------------------------------------------+---------+---------+---------------------------+
| Strength (%) / ``Strength_``               | Double  | 100     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Brightness (%) / ``Brightness_``           | Double  | 0       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Contrast (%) / ``Contrast_``               | Double  | 0       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Gamma (%) / ``Gamma_``                     | Double  | 0       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Hue (%) / ``Hue_``                         | Double  | 0       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Saturation (%) / ``Saturation_``           | Double  | 0       |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Normalize colors / ``Normalize_colors``    | Choice  | None    | |                         |
|                                            |         |         | | **None**                |
|                                            |         |         | | **Pre-process**         |
|                                            |         |         | | **Post-process**        |
|                                            |         |         | | **Both**                |
+--------------------------------------------+---------+---------+---------------------------+
| Preview type / ``Preview_type``            | Choice  | Full    | |                         |
|                                            |         |         | | **Full**                |
|                                            |         |         | | **Forward horizontal**  |
|                                            |         |         | | **Forward vertical**    |
|                                            |         |         | | **Backward horizontal** |
|                                            |         |         | | **Backward vertical**   |
|                                            |         |         | | **Duplicate top**       |
|                                            |         |         | | **Duplicate left**      |
|                                            |         |         | | **Duplicate bottom**    |
|                                            |         |         | | **Duplicate right**     |
+--------------------------------------------+---------+---------+---------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                         |
|                                            |         |         | | **Merged**              |
|                                            |         |         | | **Layer 0**             |
|                                            |         |         | | **Layer 1**             |
|                                            |         |         | | **Layer 2**             |
|                                            |         |         | | **Layer 3**             |
|                                            |         |         | | **Layer 4**             |
|                                            |         |         | | **Layer 5**             |
|                                            |         |         | | **Layer 6**             |
|                                            |         |         | | **Layer 7**             |
|                                            |         |         | | **Layer 8**             |
|                                            |         |         | | **Layer 9**             |
+--------------------------------------------+---------+---------+---------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                         |
|                                            |         |         | | **Fixed (Inplace)**     |
|                                            |         |         | | **Dynamic**             |
|                                            |         |         | | **Downsample 1/2**      |
|                                            |         |         | | **Downsample 1/4**      |
|                                            |         |         | | **Downsample 1/8**      |
|                                            |         |         | | **Downsample 1/16**     |
+--------------------------------------------+---------+---------+---------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                         |
|                                            |         |         | | **Off**                 |
|                                            |         |         | | **Level 1**             |
|                                            |         |         | | **Level 2**             |
|                                            |         |         | | **Level 3**             |
+--------------------------------------------+---------+---------+---------------------------+
