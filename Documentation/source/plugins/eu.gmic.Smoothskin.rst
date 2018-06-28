.. _eu.gmic.Smoothskin:

G’MIC Smooth skin node
======================

*This documentation is for version 0.3 of G’MIC Smooth skin.*

Description
-----------

Step 1: Skin detection

Step 2: Medium scale smoothing

Step 3: Details enhancement

Click here for a video tutorial: http://www.youtube.com/watch?v=H8pQfq-ybCc

Author: David Tschumperle. Latest update: 2013/20/12.

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

+------------------------------------------------+---------+--------------+--------------------------------+
| Parameter / script name                        | Type    | Default      | Function                       |
+================================================+=========+==============+================================+
| Skin estimation / ``Skin_estimation``          | Choice  | Automatic    | |                              |
|                                                |         |              | | **None**                     |
|                                                |         |              | | **Manual**                   |
|                                                |         |              | | **Automatic**                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Tolerance / ``Tolerance``                      | Double  | 0.5          |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Smoothness / ``Smoothness``                    | Double  | 1            |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Threshold / ``Threshold``                      | Double  | 1            |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Pre-normalize image / ``Prenormalize_image``   | Boolean | On           |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| X-coordinate [manual] / ``Xcoordinate_manual`` | Double  | 50           |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Y-coordinate [manual] / ``Ycoordinate_manual`` | Double  | 50           |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Radius [manual] / ``Radius_manual``            | Double  | 5            |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Base scale / ``Base_scale``                    | Double  | 2            |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Fine scale / ``Fine_scale``                    | Double  | 0.2          |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Smoothness_2 / ``Smoothness_2``                | Double  | 3            |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Smoothness type / ``Smoothness_type``          | Choice  | Bilateral    | |                              |
|                                                |         |              | | **Gaussian**                 |
|                                                |         |              | | **Bilateral**                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Gain / ``Gain``                                | Double  | 0.05         |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Preview data / ``Preview_data``                | Choice  | Result image | |                              |
|                                                |         |              | | **Skin mask**                |
|                                                |         |              | | **Base scale**               |
|                                                |         |              | | **Medium scale (original)**  |
|                                                |         |              | | **Medium scale (smoothed)**  |
|                                                |         |              | | **Fine scale**               |
|                                                |         |              | | **Result image**             |
+------------------------------------------------+---------+--------------+--------------------------------+
| Preview type / ``Preview_type``                | Choice  | Full         | |                              |
|                                                |         |              | | **Full**                     |
|                                                |         |              | | **Forward horizontal**       |
|                                                |         |              | | **Forward vertical**         |
|                                                |         |              | | **Backward horizontal**      |
|                                                |         |              | | **Backward vertical**        |
|                                                |         |              | | **Duplicate top**            |
|                                                |         |              | | **Duplicate left**           |
|                                                |         |              | | **Duplicate bottom**         |
|                                                |         |              | | **Duplicate right**          |
|                                                |         |              | | **Duplicate horizontal**     |
|                                                |         |              | | **Duplicate vertical**       |
|                                                |         |              | | **Checkered**                |
|                                                |         |              | | **Checkered inverse)**       |
|                                                |         |              | | **Preview split = point(50** |
|                                                |         |              | | **50**                       |
|                                                |         |              | | **0**                        |
|                                                |         |              | | **0**                        |
|                                                |         |              | | **200**                      |
|                                                |         |              | | **200**                      |
|                                                |         |              | | **200**                      |
|                                                |         |              | | **0**                        |
|                                                |         |              | | **10**                       |
|                                                |         |              | | **0**                        |
+------------------------------------------------+---------+--------------+--------------------------------+
| Output Layer / ``Output_Layer``                | Choice  | Layer 0      | |                              |
|                                                |         |              | | **Merged**                   |
|                                                |         |              | | **Layer 0**                  |
|                                                |         |              | | **Layer 1**                  |
|                                                |         |              | | **Layer 2**                  |
|                                                |         |              | | **Layer 3**                  |
|                                                |         |              | | **Layer 4**                  |
|                                                |         |              | | **Layer 5**                  |
|                                                |         |              | | **Layer 6**                  |
|                                                |         |              | | **Layer 7**                  |
|                                                |         |              | | **Layer 8**                  |
|                                                |         |              | | **Layer 9**                  |
+------------------------------------------------+---------+--------------+--------------------------------+
| Resize Mode / ``Resize_Mode``                  | Choice  | Dynamic      | |                              |
|                                                |         |              | | **Fixed (Inplace)**          |
|                                                |         |              | | **Dynamic**                  |
|                                                |         |              | | **Downsample 1/2**           |
|                                                |         |              | | **Downsample 1/4**           |
|                                                |         |              | | **Downsample 1/8**           |
|                                                |         |              | | **Downsample 1/16**          |
+------------------------------------------------+---------+--------------+--------------------------------+
| Ignore Alpha / ``Ignore_Alpha``                | Boolean | Off          |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``     | Boolean | Off          |                                |
+------------------------------------------------+---------+--------------+--------------------------------+
| Log Verbosity / ``Log_Verbosity``              | Choice  | Off          | |                              |
|                                                |         |              | | **Off**                      |
|                                                |         |              | | **Level 1**                  |
|                                                |         |              | | **Level 2**                  |
|                                                |         |              | | **Level 3**                  |
+------------------------------------------------+---------+--------------+--------------------------------+
