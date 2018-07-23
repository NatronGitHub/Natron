.. _eu.gmic.Negativenew:

G’MIC Negative new node
=======================

*This documentation is for version 1.0 of G’MIC Negative new.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

+--------+-------------+----------+
| Input  | Description | Optional |
+========+=============+==========+
| Source |             | No       |
+--------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------------------+---------+---------------+----------------------------+
| Parameter / script name                    | Type    | Default       | Function                   |
+============================================+=========+===============+============================+
| Preset / ``Preset``                        | Choice  | None          | |                          |
|                                            |         |               | | **None**                 |
|                                            |         |               | | **Fuji 160C**            |
|                                            |         |               | | **Fuji 400H**            |
|                                            |         |               | | **Fuji 800Z**            |
|                                            |         |               | | **Fuji Ilford HP5**      |
|                                            |         |               | | **Kodak Portra 160**     |
|                                            |         |               | | **Kodak Portra 400**     |
|                                            |         |               | | **Kodak Portra 800**     |
|                                            |         |               | | **Kodak TMAX 3200**      |
|                                            |         |               | | **Kodak TRI-X 400**      |
+--------------------------------------------+---------+---------------+----------------------------+
| Effect / ``Effect``                        | Choice  | Standard      | |                          |
|                                            |         |               | | **Low**                  |
|                                            |         |               | | **Standard**             |
|                                            |         |               | | **High**                 |
|                                            |         |               | | **Higher**               |
+--------------------------------------------+---------+---------------+----------------------------+
| Strength (%) / ``Strength_``               | Double  | 100           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Brightness (%) / ``Brightness_``           | Double  | 0             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Contrast (%) / ``Contrast_``               | Double  | 0             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Gamma (%) / ``Gamma_``                     | Double  | 0             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Hue (%) / ``Hue_``                         | Double  | 0             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Saturation (%) / ``Saturation_``           | Double  | 0             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Normalize colors / ``Normalize_colors``    | Choice  | None          | |                          |
|                                            |         |               | | **None**                 |
|                                            |         |               | | **Pre-process**          |
|                                            |         |               | | **Post-process**         |
|                                            |         |               | | **Both**                 |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview type / ``Preview_type``            | Choice  | Full          | |                          |
|                                            |         |               | | **Full**                 |
|                                            |         |               | | **Forward horizontal**   |
|                                            |         |               | | **Forward vertical**     |
|                                            |         |               | | **Backward horizontal**  |
|                                            |         |               | | **Backward vertical**    |
|                                            |         |               | | **Duplicate top**        |
|                                            |         |               | | **Duplicate left**       |
|                                            |         |               | | **Duplicate bottom**     |
|                                            |         |               | | **Duplicate right**      |
|                                            |         |               | | **Duplicate horizontal** |
|                                            |         |               | | **Duplicate vertical**   |
|                                            |         |               | | **Checkered**            |
|                                            |         |               | | **Checkered inverse**    |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview split / ``Preview_split``          | Double  | x: 0.5 y: 0.5 |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0       | |                          |
|                                            |         |               | | **Merged**               |
|                                            |         |               | | **Layer 0**              |
|                                            |         |               | | **Layer -1**             |
|                                            |         |               | | **Layer -2**             |
|                                            |         |               | | **Layer -3**             |
|                                            |         |               | | **Layer -4**             |
|                                            |         |               | | **Layer -5**             |
|                                            |         |               | | **Layer -6**             |
|                                            |         |               | | **Layer -7**             |
|                                            |         |               | | **Layer -8**             |
|                                            |         |               | | **Layer -9**             |
+--------------------------------------------+---------+---------------+----------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic       | |                          |
|                                            |         |               | | **Fixed (Inplace)**      |
|                                            |         |               | | **Dynamic**              |
|                                            |         |               | | **Downsample 1/2**       |
|                                            |         |               | | **Downsample 1/4**       |
|                                            |         |               | | **Downsample 1/8**       |
|                                            |         |               | | **Downsample 1/16**      |
+--------------------------------------------+---------+---------------+----------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off           | |                          |
|                                            |         |               | | **Off**                  |
|                                            |         |               | | **Level 1**              |
|                                            |         |               | | **Level 2**              |
|                                            |         |               | | **Level 3**              |
+--------------------------------------------+---------+---------------+----------------------------+
